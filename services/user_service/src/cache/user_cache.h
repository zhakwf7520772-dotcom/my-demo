#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "common/base/app_result.h"
#include "services/user_service/src/infra/redis_client.h"
#include "user_service.pb.h"

namespace my_demo {

class UserCache {
 public:
  explicit UserCache(std::shared_ptr<RedisClient> redis_client);

  AppResult<std::optional<user::v1::User>> GetUser(int64_t user_id);
  AppResult<void> SetUser(const user::v1::User& user, int ttl_seconds);
  AppResult<void> DelUser(int64_t user_id);

 private:
  static std::string BuildUserKey(int64_t user_id);

  std::shared_ptr<RedisClient> redis_client_;
};

}  // namespace my_demo
