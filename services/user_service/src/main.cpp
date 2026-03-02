#include <csignal>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "services/user_service/src/cache/user_cache.h"
#include "services/user_service/src/handler/user_rpc_handler.h"
#include "services/user_service/src/infra/config_loader.h"
#include "services/user_service/src/infra/logger.h"
#include "services/user_service/src/infra/mysql_client.h"
#include "services/user_service/src/infra/redis_client.h"
#include "services/user_service/src/repository/user_repository.h"
#include "services/user_service/src/service/user_domain_service.h"

namespace {

std::unique_ptr<grpc::Server> g_server;

void HandleSignal(int) {
  if (g_server) {
    g_server->Shutdown();
  }
}

std::string ParseConfigPath(int argc, char** argv) {
  std::string path = "./config/user_service.yaml";
  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "--config" && i + 1 < argc) {
      path = argv[++i];
    }
  }
  return path;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string config_path = ParseConfigPath(argc, argv);
  auto cfg_res = my_demo::ConfigLoader::Load(config_path);
  if (!cfg_res.ok()) {
    std::cerr << "load config failed: " << cfg_res.error().message << std::endl;
    return 1;
  }

  const my_demo::UserServiceConfig cfg = cfg_res.value();
  auto logger_res = my_demo::InitLogger(cfg.log);
  if (!logger_res.ok()) {
    std::cerr << "init logger failed: " << logger_res.error().message << std::endl;
    return 1;
  }

  auto mysql_client = std::make_shared<my_demo::MysqlClient>(cfg.mysql);
  auto mysql_connect_res = mysql_client->Connect();
  if (!mysql_connect_res.ok()) {
    my_demo::GetLogger()->error("mysql connect failed: {}", mysql_connect_res.error().message);
    return 1;
  }

  auto redis_client = std::make_shared<my_demo::RedisClient>(cfg.redis);
  auto repository = std::make_shared<my_demo::UserRepository>(mysql_client);
  auto cache = std::make_shared<my_demo::UserCache>(redis_client);
  auto domain_service =
      std::make_shared<my_demo::UserDomainService>(repository, cache, cfg.cache.user_ttl_seconds);

  my_demo::UserRpcHandler rpc_handler(domain_service);

  const std::string listen_addr = cfg.grpc.host + ":" + std::to_string(cfg.grpc.port);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(listen_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&rpc_handler);
  g_server = builder.BuildAndStart();

  if (!g_server) {
    my_demo::GetLogger()->error("failed to start grpc server at {}", listen_addr);
    return 1;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  my_demo::GetLogger()->info("user_service started at {}", listen_addr);
  g_server->Wait();
  my_demo::GetLogger()->info("user_service stopped");
  return 0;
}
