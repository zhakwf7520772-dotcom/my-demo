#include "services/user_service/src/infra/mysql_client.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "common/errors/error_code.h"

namespace my_demo {
namespace {

int ToTimeoutSeconds(int timeout_ms) {
  return std::max(1, timeout_ms / 1000);
}

}  // namespace

MysqlClient::MysqlClient(MysqlConfig config) : config_(std::move(config)) {}

MysqlClient::~MysqlClient() {
  std::lock_guard<std::mutex> lock(mu_);
  if (conn_ != nullptr) {
    mysql_close(conn_);
    conn_ = nullptr;
  }
}

AppResult<void> MysqlClient::Connect() {
  std::lock_guard<std::mutex> lock(mu_);
  return ReconnectLocked();
}

AppResult<void> MysqlClient::ReconnectLocked() {
  if (conn_ != nullptr) {
    mysql_close(conn_);
    conn_ = nullptr;
  }

  conn_ = mysql_init(nullptr);
  if (conn_ == nullptr) {
    return AppResult<void>::Err(ErrorCode::kDbError, "mysql_init failed");
  }

  const int timeout_sec = ToTimeoutSeconds(config_.connect_timeout_ms);
  mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec);

  if (mysql_real_connect(conn_,
                         config_.host.c_str(),
                         config_.user.c_str(),
                         config_.password.c_str(),
                         config_.database.c_str(),
                         static_cast<unsigned int>(config_.port),
                         nullptr,
                         0) == nullptr) {
    const unsigned int err = mysql_errno(conn_);
    const char* errmsg = mysql_error(conn_);
    return AppResult<void>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_real_connect failed", err, errmsg),
        static_cast<int>(err));
  }

  mysql_set_character_set(conn_, "utf8mb4");
  return AppResult<void>::Ok();
}

AppResult<void> MysqlClient::EnsureConnectedLocked() {
  if (conn_ == nullptr) {
    return ReconnectLocked();
  }
  if (mysql_ping(conn_) != 0) {
    return ReconnectLocked();
  }
  return AppResult<void>::Ok();
}

AppResult<int64_t> MysqlClient::InsertUser(const std::string& name, const std::string& email) {
  std::lock_guard<std::mutex> lock(mu_);
  auto ensure_res = EnsureConnectedLocked();
  if (!ensure_res.ok()) {
    return AppResult<int64_t>::Err(ensure_res.error().code, ensure_res.error().message, ensure_res.error().vendor_code);
  }

  static constexpr const char* kSql = "INSERT INTO users(name, email) VALUES(?, ?)";

  MYSQL_STMT* stmt = mysql_stmt_init(conn_);
  if (stmt == nullptr) {
    const unsigned int err = mysql_errno(conn_);
    return AppResult<int64_t>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_init failed", err, mysql_error(conn_)),
        static_cast<int>(err));
  }

  auto close_stmt = [&stmt]() {
    if (stmt != nullptr) {
      mysql_stmt_close(stmt);
      stmt = nullptr;
    }
  };

  if (mysql_stmt_prepare(stmt, kSql, std::strlen(kSql)) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<int64_t>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_prepare failed", err, errmsg),
        static_cast<int>(err));
  }

  MYSQL_BIND binds[2];
  std::memset(binds, 0, sizeof(binds));

  unsigned long name_len = static_cast<unsigned long>(name.size());
  unsigned long email_len = static_cast<unsigned long>(email.size());

  binds[0].buffer_type = MYSQL_TYPE_STRING;
  binds[0].buffer = const_cast<char*>(name.data());
  binds[0].buffer_length = name_len;
  binds[0].length = &name_len;

  binds[1].buffer_type = MYSQL_TYPE_STRING;
  binds[1].buffer = const_cast<char*>(email.data());
  binds[1].buffer_length = email_len;
  binds[1].length = &email_len;

  if (mysql_stmt_bind_param(stmt, binds) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<int64_t>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_bind_param failed", err, errmsg),
        static_cast<int>(err));
  }

  if (mysql_stmt_execute(stmt) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<int64_t>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_execute failed", err, errmsg),
        static_cast<int>(err));
  }

  const int64_t id = static_cast<int64_t>(mysql_stmt_insert_id(stmt));
  close_stmt();
  return AppResult<int64_t>::Ok(id);
}

AppResult<std::optional<MysqlUserRow>> MysqlClient::GetUserById(int64_t user_id) {
  std::lock_guard<std::mutex> lock(mu_);
  auto ensure_res = EnsureConnectedLocked();
  if (!ensure_res.ok()) {
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ensure_res.error().code,
        ensure_res.error().message,
        ensure_res.error().vendor_code);
  }

  static constexpr const char* kSql =
      "SELECT id, name, email, "
      "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s'), "
      "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') "
      "FROM users WHERE id = ? LIMIT 1";

  MYSQL_STMT* stmt = mysql_stmt_init(conn_);
  if (stmt == nullptr) {
    const unsigned int err = mysql_errno(conn_);
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_init failed", err, mysql_error(conn_)),
        static_cast<int>(err));
  }

  auto close_stmt = [&stmt]() {
    if (stmt != nullptr) {
      mysql_stmt_close(stmt);
      stmt = nullptr;
    }
  };

  if (mysql_stmt_prepare(stmt, kSql, std::strlen(kSql)) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_prepare failed", err, errmsg),
        static_cast<int>(err));
  }

  MYSQL_BIND param_bind[1];
  std::memset(param_bind, 0, sizeof(param_bind));

  long long id_param = static_cast<long long>(user_id);
  param_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
  param_bind[0].buffer = &id_param;
  param_bind[0].is_unsigned = 0;

  if (mysql_stmt_bind_param(stmt, param_bind) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_bind_param failed", err, errmsg),
        static_cast<int>(err));
  }

  if (mysql_stmt_execute(stmt) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_execute failed", err, errmsg),
        static_cast<int>(err));
  }

  std::array<char, 65> name_buf{};
  std::array<char, 129> email_buf{};
  std::array<char, 32> created_buf{};
  std::array<char, 32> updated_buf{};

  long long out_id = 0;
  unsigned long name_len = 0;
  unsigned long email_len = 0;
  unsigned long created_len = 0;
  unsigned long updated_len = 0;

  MYSQL_BIND result_bind[5];
  std::memset(result_bind, 0, sizeof(result_bind));

  result_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
  result_bind[0].buffer = &out_id;
  result_bind[0].is_unsigned = 0;

  result_bind[1].buffer_type = MYSQL_TYPE_STRING;
  result_bind[1].buffer = name_buf.data();
  result_bind[1].buffer_length = name_buf.size();
  result_bind[1].length = &name_len;

  result_bind[2].buffer_type = MYSQL_TYPE_STRING;
  result_bind[2].buffer = email_buf.data();
  result_bind[2].buffer_length = email_buf.size();
  result_bind[2].length = &email_len;

  result_bind[3].buffer_type = MYSQL_TYPE_STRING;
  result_bind[3].buffer = created_buf.data();
  result_bind[3].buffer_length = created_buf.size();
  result_bind[3].length = &created_len;

  result_bind[4].buffer_type = MYSQL_TYPE_STRING;
  result_bind[4].buffer = updated_buf.data();
  result_bind[4].buffer_length = updated_buf.size();
  result_bind[4].length = &updated_len;

  if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_bind_result failed", err, errmsg),
        static_cast<int>(err));
  }

  if (mysql_stmt_store_result(stmt) != 0) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_store_result failed", err, errmsg),
        static_cast<int>(err));
  }

  const int fetch_rc = mysql_stmt_fetch(stmt);
  if (fetch_rc == MYSQL_NO_DATA) {
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Ok(std::nullopt);
  }
  if (fetch_rc == 1 || fetch_rc == MYSQL_DATA_TRUNCATED) {
    const unsigned int err = mysql_stmt_errno(stmt);
    const char* errmsg = mysql_stmt_error(stmt);
    close_stmt();
    return AppResult<std::optional<MysqlUserRow>>::Err(
        ErrorCode::kDbError,
        BuildDbError("mysql_stmt_fetch failed", err, errmsg),
        static_cast<int>(err));
  }

  MysqlUserRow row;
  row.id = static_cast<int64_t>(out_id);
  row.name.assign(name_buf.data(), name_len);
  row.email.assign(email_buf.data(), email_len);
  row.created_at.assign(created_buf.data(), created_len);
  row.updated_at.assign(updated_buf.data(), updated_len);

  close_stmt();
  return AppResult<std::optional<MysqlUserRow>>::Ok(row);
}

std::string MysqlClient::BuildDbError(const std::string& prefix,
                                      unsigned int mysql_errno,
                                      const char* mysql_errmsg) const {
  return prefix + ", errno=" + std::to_string(mysql_errno) + ", errmsg=" +
         (mysql_errmsg == nullptr ? "" : std::string(mysql_errmsg));
}

}  // namespace my_demo
