#pragma once

#include <string>

namespace my_demo {

struct GrpcConfig {
  std::string host = "0.0.0.0";
  int port = 50051;
};

struct MysqlConfig {
  std::string host = "127.0.0.1";
  int port = 13306;
  std::string user = "root";
  std::string password = "root";
  std::string database = "demo";
  int connect_timeout_ms = 2000;
};

struct RedisConfig {
  std::string host = "127.0.0.1";
  int port = 16379;
  std::string password;
  int db = 0;
  int timeout_ms = 1500;
};

struct CacheConfig {
  int user_ttl_seconds = 300;
};

struct LogConfig {
  std::string level = "info";
  std::string log_path;
};

struct UserServiceConfig {
  GrpcConfig grpc;
  MysqlConfig mysql;
  RedisConfig redis;
  CacheConfig cache;
  LogConfig log;
};

}  // namespace my_demo
