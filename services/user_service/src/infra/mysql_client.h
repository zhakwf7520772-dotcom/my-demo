#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

#include "common/base/app_result.h"
#include "services/user_service/src/infra/config_types.h"
#include "services/user_service/src/infra/mysql_headers.h"

namespace my_demo {

struct MysqlUserRow {
  int64_t id = 0;
  std::string name;
  std::string email;
  std::string created_at;
  std::string updated_at;
};

class MysqlClient {
 public:
  explicit MysqlClient(MysqlConfig config);
  ~MysqlClient();

  AppResult<void> Connect();
  AppResult<int64_t> InsertUser(const std::string& name, const std::string& email);
  AppResult<std::optional<MysqlUserRow>> GetUserById(int64_t user_id);

 private:
  AppResult<void> EnsureConnectedLocked();
  AppResult<void> ReconnectLocked();
  std::string BuildDbError(const std::string& prefix, unsigned int mysql_errno, const char* mysql_errmsg) const;

  MysqlConfig config_;
  MYSQL* conn_ = nullptr;
  std::mutex mu_;
};

}  // namespace my_demo
