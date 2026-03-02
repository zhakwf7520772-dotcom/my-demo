#include "services/user_service/src/infra/config_loader.h"

#include <cstdlib>
#include <exception>
#include <string>

#include <yaml-cpp/yaml.h>

#include "common/errors/error_code.h"

namespace my_demo {
namespace {

std::string GetEnvString(const char* key) {
  const char* value = std::getenv(key);
  return value == nullptr ? "" : std::string(value);
}

void ApplyStringOverride(const char* key, std::string* target) {
  const std::string value = GetEnvString(key);
  if (!value.empty()) {
    *target = value;
  }
}

void ApplyIntOverride(const char* key, int* target) {
  const std::string value = GetEnvString(key);
  if (value.empty()) {
    return;
  }
  try {
    *target = std::stoi(value);
  } catch (const std::exception&) {
    // Ignore invalid overrides to keep startup behavior predictable.
  }
}

void LoadYaml(const YAML::Node& root, UserServiceConfig* cfg) {
  if (root["grpc"]) {
    const auto grpc = root["grpc"];
    if (grpc["host"]) cfg->grpc.host = grpc["host"].as<std::string>();
    if (grpc["port"]) cfg->grpc.port = grpc["port"].as<int>();
  }

  if (root["mysql"]) {
    const auto mysql = root["mysql"];
    if (mysql["host"]) cfg->mysql.host = mysql["host"].as<std::string>();
    if (mysql["port"]) cfg->mysql.port = mysql["port"].as<int>();
    if (mysql["user"]) cfg->mysql.user = mysql["user"].as<std::string>();
    if (mysql["password"]) cfg->mysql.password = mysql["password"].as<std::string>();
    if (mysql["database"]) cfg->mysql.database = mysql["database"].as<std::string>();
    if (mysql["connect_timeout_ms"]) cfg->mysql.connect_timeout_ms = mysql["connect_timeout_ms"].as<int>();
  }

  if (root["redis"]) {
    const auto redis = root["redis"];
    if (redis["host"]) cfg->redis.host = redis["host"].as<std::string>();
    if (redis["port"]) cfg->redis.port = redis["port"].as<int>();
    if (redis["password"]) cfg->redis.password = redis["password"].as<std::string>();
    if (redis["db"]) cfg->redis.db = redis["db"].as<int>();
    if (redis["timeout_ms"]) cfg->redis.timeout_ms = redis["timeout_ms"].as<int>();
  }

  if (root["cache"]) {
    const auto cache = root["cache"];
    if (cache["user_ttl_seconds"]) cfg->cache.user_ttl_seconds = cache["user_ttl_seconds"].as<int>();
  }

  if (root["log"]) {
    const auto log = root["log"];
    if (log["level"]) cfg->log.level = log["level"].as<std::string>();
    if (log["log_path"]) cfg->log.log_path = log["log_path"].as<std::string>();
  }
}

void ApplyEnvOverrides(UserServiceConfig* cfg) {
  ApplyStringOverride("USER_SVC__GRPC__HOST", &cfg->grpc.host);
  ApplyIntOverride("USER_SVC__GRPC__PORT", &cfg->grpc.port);

  ApplyStringOverride("USER_SVC__MYSQL__HOST", &cfg->mysql.host);
  ApplyIntOverride("USER_SVC__MYSQL__PORT", &cfg->mysql.port);
  ApplyStringOverride("USER_SVC__MYSQL__USER", &cfg->mysql.user);
  ApplyStringOverride("USER_SVC__MYSQL__PASSWORD", &cfg->mysql.password);
  ApplyStringOverride("USER_SVC__MYSQL__DATABASE", &cfg->mysql.database);
  ApplyIntOverride("USER_SVC__MYSQL__CONNECT_TIMEOUT_MS", &cfg->mysql.connect_timeout_ms);

  ApplyStringOverride("USER_SVC__REDIS__HOST", &cfg->redis.host);
  ApplyIntOverride("USER_SVC__REDIS__PORT", &cfg->redis.port);
  ApplyStringOverride("USER_SVC__REDIS__PASSWORD", &cfg->redis.password);
  ApplyIntOverride("USER_SVC__REDIS__DB", &cfg->redis.db);
  ApplyIntOverride("USER_SVC__REDIS__TIMEOUT_MS", &cfg->redis.timeout_ms);

  ApplyIntOverride("USER_SVC__CACHE__USER_TTL_SECONDS", &cfg->cache.user_ttl_seconds);

  ApplyStringOverride("USER_SVC__LOG__LEVEL", &cfg->log.level);
  ApplyStringOverride("USER_SVC__LOG__LOG_PATH", &cfg->log.log_path);
}

bool Validate(const UserServiceConfig& cfg, std::string* reason) {
  if (cfg.grpc.port <= 0) {
    *reason = "grpc.port must be positive";
    return false;
  }
  if (cfg.mysql.host.empty() || cfg.mysql.user.empty() || cfg.mysql.database.empty()) {
    *reason = "mysql.host/user/database must not be empty";
    return false;
  }
  if (cfg.mysql.port <= 0) {
    *reason = "mysql.port must be positive";
    return false;
  }
  if (cfg.redis.host.empty() || cfg.redis.port <= 0) {
    *reason = "redis.host/port invalid";
    return false;
  }
  if (cfg.cache.user_ttl_seconds <= 0) {
    *reason = "cache.user_ttl_seconds must be positive";
    return false;
  }
  return true;
}

}  // namespace

AppResult<UserServiceConfig> ConfigLoader::Load(const std::string& file_path) {
  UserServiceConfig cfg;

  try {
    const YAML::Node root = YAML::LoadFile(file_path);
    LoadYaml(root, &cfg);
  } catch (const std::exception& ex) {
    return AppResult<UserServiceConfig>::Err(
        ErrorCode::kInvalidArgument,
        std::string("failed to load config file '") + file_path + "': " + ex.what());
  }

  ApplyEnvOverrides(&cfg);

  std::string reason;
  if (!Validate(cfg, &reason)) {
    return AppResult<UserServiceConfig>::Err(
        ErrorCode::kInvalidArgument,
        std::string("invalid config: ") + reason);
  }

  return AppResult<UserServiceConfig>::Ok(cfg);
}

}  // namespace my_demo
