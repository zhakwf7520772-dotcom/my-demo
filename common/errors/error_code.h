#pragma once

#include <cstdint>

namespace my_demo {

enum class ErrorCode : int32_t {
  kOk = 0,
  kInvalidArgument = 40001,
  kUserNotFound = 40401,
  kEmailConflict = 40901,
  kDbError = 50001,
  kRedisError = 50002,
  kDownstreamRpcError = 50003,
};

inline int32_t ToInt(ErrorCode code) {
  return static_cast<int32_t>(code);
}

inline const char* ToMessage(ErrorCode code) {
  switch (code) {
    case ErrorCode::kOk:
      return "ok";
    case ErrorCode::kInvalidArgument:
      return "invalid argument";
    case ErrorCode::kUserNotFound:
      return "user not found";
    case ErrorCode::kEmailConflict:
      return "email already exists";
    case ErrorCode::kDbError:
      return "database error";
    case ErrorCode::kRedisError:
      return "redis error";
    case ErrorCode::kDownstreamRpcError:
      return "downstream rpc error";
    default:
      return "unknown error";
  }
}

}  // namespace my_demo
