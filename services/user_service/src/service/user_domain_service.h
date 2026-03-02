#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "common/base/app_result.h"
#include "common/base/request_context.h"
#include "services/user_service/src/cache/user_cache.h"
#include "services/user_service/src/repository/user_repository.h"
#include "user_service.pb.h"

namespace my_demo {

struct CreateUserInput {
  std::string name;
  std::string email;
};

struct CreateUserOutput {
  int64_t user_id = 0;
  std::string created_at;
};

struct GetUserOutput {
  user::v1::User user;
  bool cache_hit = false;
};

class UserDomainService {
 public:
  UserDomainService(std::shared_ptr<UserRepository> repository,
                    std::shared_ptr<UserCache> cache,
                    int user_cache_ttl_seconds);

  AppResult<CreateUserOutput> CreateUser(const CreateUserInput& input, const RequestContext& ctx);
  AppResult<GetUserOutput> GetUser(int64_t user_id, const RequestContext& ctx);

 private:
  bool IsValidName(const std::string& name) const;
  bool IsValidEmail(const std::string& email) const;
  user::v1::User ToProtoUser(const UserEntity& entity) const;

  std::shared_ptr<UserRepository> repository_;
  std::shared_ptr<UserCache> cache_;
  int user_cache_ttl_seconds_ = 300;
};

}  // namespace my_demo
