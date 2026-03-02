#include "services/user_service/src/cache/user_cache.h"

#include <utility>

#include "common/errors/error_code.h"

namespace my_demo {

UserCache::UserCache(std::shared_ptr<RedisClient> redis_client)
    : redis_client_(std::move(redis_client)) {}

AppResult<std::optional<user::v1::User>> UserCache::GetUser(int64_t user_id) {
  auto get_res = redis_client_->Get(BuildUserKey(user_id));
  if (!get_res.ok()) {
    return AppResult<std::optional<user::v1::User>>::Err(
        ErrorCode::kRedisError,
        get_res.error().message,
        get_res.error().vendor_code);
  }

  if (!get_res.value().has_value()) {
    return AppResult<std::optional<user::v1::User>>::Ok(std::nullopt);
  }

  user::v1::User user;
  if (!user.ParseFromString(get_res.value().value())) {
    return AppResult<std::optional<user::v1::User>>::Err(
        ErrorCode::kRedisError,
        "failed to parse cached user protobuf");
  }

  return AppResult<std::optional<user::v1::User>>::Ok(user);
}

AppResult<void> UserCache::SetUser(const user::v1::User& user, int ttl_seconds) {
  std::string payload;
  if (!user.SerializeToString(&payload)) {
    return AppResult<void>::Err(ErrorCode::kRedisError, "failed to serialize user protobuf");
  }
  return redis_client_->SetEx(BuildUserKey(user.id()), ttl_seconds, payload);
}

AppResult<void> UserCache::DelUser(int64_t user_id) {
  return redis_client_->Del(BuildUserKey(user_id));
}

std::string UserCache::BuildUserKey(int64_t user_id) {
  return "user:" + std::to_string(user_id);
}

}  // namespace my_demo
