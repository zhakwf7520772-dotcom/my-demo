#pragma once

#include <memory>
#include <optional>
#include <string>

#include "common/base/app_result.h"
#include "services/user_service/src/infra/config_types.h"
#include "services/user_service/src/infra/redis_headers.h"

namespace my_demo {

class RedisClient {
 public:
  explicit RedisClient(RedisConfig config);

  AppResult<std::optional<std::string>> Get(const std::string& key);
  AppResult<void> SetEx(const std::string& key, int ttl_seconds, const std::string& value);
  AppResult<void> Del(const std::string& key);

 private:
  struct RedisContextDeleter {
    void operator()(redisContext* ctx) const {
      if (ctx != nullptr) {
        redisFree(ctx);
      }
    }
  };

  using RedisContextPtr = std::unique_ptr<redisContext, RedisContextDeleter>;

  AppResult<RedisContextPtr> Connect();

  RedisConfig config_;
};

}  // namespace my_demo
