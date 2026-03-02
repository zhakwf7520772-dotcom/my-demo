#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "common/base/app_result.h"
#include "services/user_service/src/infra/mysql_client.h"

namespace my_demo {

struct UserEntity {
  int64_t id = 0;
  std::string name;
  std::string email;
  std::string created_at;
  std::string updated_at;
};

class UserRepository {
 public:
  explicit UserRepository(std::shared_ptr<MysqlClient> mysql_client);

  AppResult<UserEntity> CreateUser(const std::string& name, const std::string& email);
  AppResult<UserEntity> GetUserById(int64_t user_id);

 private:
  std::shared_ptr<MysqlClient> mysql_client_;
};

}  // namespace my_demo
