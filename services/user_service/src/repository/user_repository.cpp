#include "services/user_service/src/repository/user_repository.h"

#include <utility>

#include "common/errors/error_code.h"

namespace my_demo {

UserRepository::UserRepository(std::shared_ptr<MysqlClient> mysql_client)
    : mysql_client_(std::move(mysql_client)) {}

AppResult<UserEntity> UserRepository::CreateUser(const std::string& name, const std::string& email) {
  auto insert_res = mysql_client_->InsertUser(name, email);
  if (!insert_res.ok()) {
    if (insert_res.error().vendor_code == 1062) {
      return AppResult<UserEntity>::Err(ErrorCode::kEmailConflict, "email already exists", 1062);
    }
    return AppResult<UserEntity>::Err(
        ErrorCode::kDbError,
        insert_res.error().message,
        insert_res.error().vendor_code);
  }

  return GetUserById(insert_res.value());
}

AppResult<UserEntity> UserRepository::GetUserById(int64_t user_id) {
  auto get_res = mysql_client_->GetUserById(user_id);
  if (!get_res.ok()) {
    return AppResult<UserEntity>::Err(
        ErrorCode::kDbError,
        get_res.error().message,
        get_res.error().vendor_code);
  }

  if (!get_res.value().has_value()) {
    return AppResult<UserEntity>::Err(ErrorCode::kUserNotFound, "user not found");
  }

  const auto& row = get_res.value().value();
  UserEntity entity;
  entity.id = row.id;
  entity.name = row.name;
  entity.email = row.email;
  entity.created_at = row.created_at;
  entity.updated_at = row.updated_at;
  return AppResult<UserEntity>::Ok(entity);
}

}  // namespace my_demo
