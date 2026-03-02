#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

#include "services/user_service/src/service/user_domain_service.h"
#include "user_service.grpc.pb.h"

namespace my_demo {

class UserRpcHandler final : public user::v1::UserService::Service {
 public:
  explicit UserRpcHandler(std::shared_ptr<UserDomainService> domain_service);

  grpc::Status CreateUser(grpc::ServerContext* context,
                          const user::v1::CreateUserRequest* request,
                          user::v1::CreateUserResponse* response) override;

  grpc::Status GetUser(grpc::ServerContext* context,
                       const user::v1::GetUserRequest* request,
                       user::v1::GetUserResponse* response) override;

 private:
  RequestContext BuildRequestContext(grpc::ServerContext* context, const char* method) const;

  std::shared_ptr<UserDomainService> domain_service_;
};

}  // namespace my_demo
