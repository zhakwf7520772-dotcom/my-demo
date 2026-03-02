#include "services/user_service/src/service/user_domain_service.h"

#include <regex>
#include <utility>

#include "common/errors/error_code.h"
#include "services/user_service/src/infra/logger.h"

namespace my_demo {
namespace {

const std::regex& EmailRegex() {
  static const std::regex kRegex(R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)", std::regex::ECMAScript);
  return kRegex;
}

}  // namespace

UserDomainService::UserDomainService(std::shared_ptr<UserRepository> repository,
                                     std::shared_ptr<UserCache> cache,
                                     int user_cache_ttl_seconds)
    : repository_(std::move(repository)),
      cache_(std::move(cache)),
      user_cache_ttl_seconds_(user_cache_ttl_seconds) {}

AppResult<CreateUserOutput> UserDomainService::CreateUser(const CreateUserInput& input,
                                                          const RequestContext& ctx) {
  if (!IsValidName(input.name) || !IsValidEmail(input.email)) {
    return AppResult<CreateUserOutput>::Err(
        ErrorCode::kInvalidArgument,
        "invalid name or email");
  }

  auto repo_res = repository_->CreateUser(input.name, input.email);
  if (!repo_res.ok()) {
    return AppResult<CreateUserOutput>::Err(
        repo_res.error().code,
        repo_res.error().message,
        repo_res.error().vendor_code);
  }

  const auto& entity = repo_res.value();
  const auto proto_user = ToProtoUser(entity);
  auto cache_res = cache_->SetUser(proto_user, user_cache_ttl_seconds_);
  if (!cache_res.ok()) {
    GetLogger()->warn(
        "trace_id={} cache_set_failed key=user:{} err={}",
        ctx.trace_id,
        entity.id,
        cache_res.error().message);
  }

  CreateUserOutput output;
  output.user_id = entity.id;
  output.created_at = entity.created_at;
  return AppResult<CreateUserOutput>::Ok(output);
}

AppResult<GetUserOutput> UserDomainService::GetUser(int64_t user_id, const RequestContext& ctx) {
  if (user_id <= 0) {
    return AppResult<GetUserOutput>::Err(ErrorCode::kInvalidArgument, "user_id must be positive");
  }

  auto cache_res = cache_->GetUser(user_id);
  if (cache_res.ok() && cache_res.value().has_value()) {
    GetUserOutput output;
    output.user = cache_res.value().value();
    output.cache_hit = true;
    return AppResult<GetUserOutput>::Ok(output);
  }

  if (!cache_res.ok()) {
    GetLogger()->warn(
        "trace_id={} cache_get_failed key=user:{} err={}",
        ctx.trace_id,
        user_id,
        cache_res.error().message);
  }

  auto repo_res = repository_->GetUserById(user_id);
  if (!repo_res.ok()) {
    return AppResult<GetUserOutput>::Err(
        repo_res.error().code,
        repo_res.error().message,
        repo_res.error().vendor_code);
  }

  GetUserOutput output;
  output.user = ToProtoUser(repo_res.value());
  output.cache_hit = false;

  auto set_cache_res = cache_->SetUser(output.user, user_cache_ttl_seconds_);
  if (!set_cache_res.ok()) {
    GetLogger()->warn(
        "trace_id={} cache_set_failed key=user:{} err={}",
        ctx.trace_id,
        user_id,
        set_cache_res.error().message);
  }

  return AppResult<GetUserOutput>::Ok(output);
}

bool UserDomainService::IsValidName(const std::string& name) const {
  return !name.empty() && name.size() <= 64;
}

bool UserDomainService::IsValidEmail(const std::string& email) const {
  if (email.empty() || email.size() > 128) {
    return false;
  }
  return std::regex_match(email, EmailRegex());
}

user::v1::User UserDomainService::ToProtoUser(const UserEntity& entity) const {
  user::v1::User user;
  user.set_id(entity.id);
  user.set_name(entity.name);
  user.set_email(entity.email);
  user.set_created_at(entity.created_at);
  return user;
}

}  // namespace my_demo
