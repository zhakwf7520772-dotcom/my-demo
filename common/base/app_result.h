#pragma once

#include <optional>
#include <string>
#include <utility>

#include "common/errors/error_code.h"

namespace my_demo {

struct AppError {
  ErrorCode code = ErrorCode::kOk;
  std::string message = "ok";
  int vendor_code = 0;
};

template <typename T>
class AppResult {
 public:
  static AppResult<T> Ok(T value) {
    AppResult<T> result;
    result.ok_ = true;
    result.value_ = std::move(value);
    result.error_ = {ErrorCode::kOk, "ok", 0};
    return result;
  }

  static AppResult<T> Err(ErrorCode code, std::string message, int vendor_code = 0) {
    AppResult<T> result;
    result.ok_ = false;
    result.value_.reset();
    result.error_ = {code, std::move(message), vendor_code};
    return result;
  }

  bool ok() const { return ok_; }

  const T& value() const { return value_.value(); }
  T& value() { return value_.value(); }

  const AppError& error() const { return error_; }

 private:
  bool ok_ = false;
  std::optional<T> value_;
  AppError error_;
};

template <>
class AppResult<void> {
 public:
  static AppResult<void> Ok() {
    AppResult<void> result;
    result.ok_ = true;
    result.error_ = {ErrorCode::kOk, "ok", 0};
    return result;
  }

  static AppResult<void> Err(ErrorCode code, std::string message, int vendor_code = 0) {
    AppResult<void> result;
    result.ok_ = false;
    result.error_ = {code, std::move(message), vendor_code};
    return result;
  }

  bool ok() const { return ok_; }
  const AppError& error() const { return error_; }

 private:
  bool ok_ = false;
  AppError error_;
};

}  // namespace my_demo
