#include "services/user_service/src/handler/user_rpc_handler.h"

#include <chrono>
#include <cstdint>
#include <string>

#include "common/errors/error_code.h"
#include "services/user_service/src/infra/logger.h"

namespace my_demo {
namespace {

std::string MakeTraceId() {
  const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  return "trace-" + std::to_string(now_ns);
}

std::string ReadMetadata(grpc::ServerContext* context, const std::string& key) {
  const auto it = context->client_metadata().find(key);
  if (it == context->client_metadata().end()) {
    return "";
  }
  return std::string(it->second.data(), it->second.length());
}

}  // namespace

UserRpcHandler::UserRpcHandler(std::shared_ptr<UserDomainService> domain_service)
    : domain_service_(std::move(domain_service)) {}

grpc::Status UserRpcHandler::CreateUser(grpc::ServerContext* context,
                                        const user::v1::CreateUserRequest* request,
                                        user::v1::CreateUserResponse* response) {
  const auto started = std::chrono::steady_clock::now();
  const RequestContext req_ctx = BuildRequestContext(context, "CreateUser");

  auto result = domain_service_->CreateUser({request->name(), request->email()}, req_ctx);

  if (result.ok()) {
    response->set_code(ToInt(ErrorCode::kOk));
    response->set_message(ToMessage(ErrorCode::kOk));
    response->set_user_id(result.value().user_id);
    response->set_created_at(result.value().created_at);
  } else {
    response->set_code(ToInt(result.error().code));
    response->set_message(result.error().message);
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - started)
                              .count();

  GetLogger()->info(
      "trace_id={} method={} peer={} name={} email={} code={} cost_ms={}",
      req_ctx.trace_id,
      req_ctx.method,
      req_ctx.peer,
      request->name(),
      request->email(),
      response->code(),
      elapsed_ms);

  return grpc::Status::OK;
}

grpc::Status UserRpcHandler::GetUser(grpc::ServerContext* context,
                                     const user::v1::GetUserRequest* request,
                                     user::v1::GetUserResponse* response) {
  const auto started = std::chrono::steady_clock::now();
  const RequestContext req_ctx = BuildRequestContext(context, "GetUser");

  auto result = domain_service_->GetUser(request->user_id(), req_ctx);

  bool cache_hit = false;
  if (result.ok()) {
    response->set_code(ToInt(ErrorCode::kOk));
    response->set_message(ToMessage(ErrorCode::kOk));
    *response->mutable_user() = result.value().user;
    cache_hit = result.value().cache_hit;
  } else {
    response->set_code(ToInt(result.error().code));
    response->set_message(result.error().message);
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - started)
                              .count();

  GetLogger()->info(
      "trace_id={} method={} peer={} user_id={} code={} cache_hit={} cost_ms={}",
      req_ctx.trace_id,
      req_ctx.method,
      req_ctx.peer,
      request->user_id(),
      response->code(),
      cache_hit ? "true" : "false",
      elapsed_ms);

  return grpc::Status::OK;
}

RequestContext UserRpcHandler::BuildRequestContext(grpc::ServerContext* context, const char* method) const {
  RequestContext ctx;
  ctx.trace_id = ReadMetadata(context, "x-trace-id");
  if (ctx.trace_id.empty()) {
    ctx.trace_id = MakeTraceId();
  }
  ctx.peer = context->peer();
  ctx.method = method;
  return ctx;
}

}  // namespace my_demo
