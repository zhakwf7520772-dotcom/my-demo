#include "services/user_service/src/infra/redis_client.h"

#include <sys/time.h>

#include <string>

#include "common/errors/error_code.h"

namespace my_demo {

RedisClient::RedisClient(RedisConfig config) : config_(std::move(config)) {}

AppResult<RedisClient::RedisContextPtr> RedisClient::Connect() {
  timeval timeout;
  timeout.tv_sec = config_.timeout_ms / 1000;
  timeout.tv_usec = (config_.timeout_ms % 1000) * 1000;
  if (timeout.tv_sec <= 0 && timeout.tv_usec <= 0) {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
  }

  redisContext* raw = redisConnectWithTimeout(config_.host.c_str(), config_.port, timeout);
  if (raw == nullptr) {
    return AppResult<RedisContextPtr>::Err(ErrorCode::kRedisError, "redisConnectWithTimeout returned null");
  }

  RedisContextPtr ctx(raw);

  if (ctx->err != 0) {
    return AppResult<RedisContextPtr>::Err(
        ErrorCode::kRedisError,
        std::string("redis connect failed: ") + ctx->errstr,
        ctx->err);
  }

  if (!config_.password.empty()) {
    redisReply* auth_reply = static_cast<redisReply*>(
        redisCommand(ctx.get(), "AUTH %b", config_.password.data(), config_.password.size()));
    if (auth_reply == nullptr) {
      return AppResult<RedisContextPtr>::Err(ErrorCode::kRedisError, "redis AUTH failed: null reply");
    }
    std::unique_ptr<redisReply, decltype(&freeReplyObject)> auth_guard(auth_reply, freeReplyObject);
    if (auth_reply->type == REDIS_REPLY_ERROR) {
      return AppResult<RedisContextPtr>::Err(
          ErrorCode::kRedisError,
          std::string("redis AUTH error: ") + (auth_reply->str == nullptr ? "" : auth_reply->str));
    }
  }

  if (config_.db > 0) {
    redisReply* select_reply = static_cast<redisReply*>(redisCommand(ctx.get(), "SELECT %d", config_.db));
    if (select_reply == nullptr) {
      return AppResult<RedisContextPtr>::Err(ErrorCode::kRedisError, "redis SELECT failed: null reply");
    }
    std::unique_ptr<redisReply, decltype(&freeReplyObject)> select_guard(select_reply, freeReplyObject);
    if (select_reply->type == REDIS_REPLY_ERROR) {
      return AppResult<RedisContextPtr>::Err(
          ErrorCode::kRedisError,
          std::string("redis SELECT error: ") + (select_reply->str == nullptr ? "" : select_reply->str));
    }
  }

  return AppResult<RedisContextPtr>::Ok(std::move(ctx));
}

AppResult<std::optional<std::string>> RedisClient::Get(const std::string& key) {
  auto conn_res = Connect();
  if (!conn_res.ok()) {
    return AppResult<std::optional<std::string>>::Err(
        conn_res.error().code,
        conn_res.error().message,
        conn_res.error().vendor_code);
  }

  RedisContextPtr ctx = std::move(conn_res.value());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(ctx.get(), "GET %b", key.data(), key.size()));
  if (reply == nullptr) {
    return AppResult<std::optional<std::string>>::Err(
        ErrorCode::kRedisError,
        std::string("redis GET failed: ") + (ctx->errstr == nullptr ? "" : ctx->errstr),
        ctx->err);
  }

  std::unique_ptr<redisReply, decltype(&freeReplyObject)> guard(reply, freeReplyObject);

  if (reply->type == REDIS_REPLY_NIL) {
    return AppResult<std::optional<std::string>>::Ok(std::nullopt);
  }
  if (reply->type != REDIS_REPLY_STRING) {
    return AppResult<std::optional<std::string>>::Err(
        ErrorCode::kRedisError,
        "redis GET returned unexpected reply type");
  }

  return AppResult<std::optional<std::string>>::Ok(std::string(reply->str, reply->len));
}

AppResult<void> RedisClient::SetEx(const std::string& key, int ttl_seconds, const std::string& value) {
  auto conn_res = Connect();
  if (!conn_res.ok()) {
    return AppResult<void>::Err(conn_res.error().code, conn_res.error().message, conn_res.error().vendor_code);
  }

  RedisContextPtr ctx = std::move(conn_res.value());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(ctx.get(), "SETEX %b %d %b", key.data(), key.size(), ttl_seconds, value.data(), value.size()));
  if (reply == nullptr) {
    return AppResult<void>::Err(
        ErrorCode::kRedisError,
        std::string("redis SETEX failed: ") + (ctx->errstr == nullptr ? "" : ctx->errstr),
        ctx->err);
  }

  std::unique_ptr<redisReply, decltype(&freeReplyObject)> guard(reply, freeReplyObject);

  if (reply->type == REDIS_REPLY_ERROR) {
    return AppResult<void>::Err(
        ErrorCode::kRedisError,
        std::string("redis SETEX error: ") + (reply->str == nullptr ? "" : reply->str));
  }

  return AppResult<void>::Ok();
}

AppResult<void> RedisClient::Del(const std::string& key) {
  auto conn_res = Connect();
  if (!conn_res.ok()) {
    return AppResult<void>::Err(conn_res.error().code, conn_res.error().message, conn_res.error().vendor_code);
  }

  RedisContextPtr ctx = std::move(conn_res.value());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(ctx.get(), "DEL %b", key.data(), key.size()));
  if (reply == nullptr) {
    return AppResult<void>::Err(
        ErrorCode::kRedisError,
        std::string("redis DEL failed: ") + (ctx->errstr == nullptr ? "" : ctx->errstr),
        ctx->err);
  }

  std::unique_ptr<redisReply, decltype(&freeReplyObject)> guard(reply, freeReplyObject);
  if (reply->type == REDIS_REPLY_ERROR) {
    return AppResult<void>::Err(
        ErrorCode::kRedisError,
        std::string("redis DEL error: ") + (reply->str == nullptr ? "" : reply->str));
  }

  return AppResult<void>::Ok();
}

}  // namespace my_demo
