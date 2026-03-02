#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "user_service.grpc.pb.h"

namespace {

void PrintUsage() {
  std::cout << "Usage:\n"
            << "  user_client [--target host:port] create --name <name> --email <email>\n"
            << "  user_client [--target host:port] get --user_id <id>\n";
}

std::optional<std::string> GetArgValue(const std::vector<std::string>& args, const std::string& key) {
  for (size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == key) {
      return args[i + 1];
    }
  }
  return std::nullopt;
}

int32_t ExtractCodeFromGrpcStatus(const grpc::Status& status) {
  if (status.ok()) {
    return 0;
  }
  return 50003;
}

int RunCreate(user::v1::UserService::Stub* stub, const std::vector<std::string>& args) {
  const auto name_opt = GetArgValue(args, "--name");
  const auto email_opt = GetArgValue(args, "--email");
  if (!name_opt.has_value() || !email_opt.has_value()) {
    PrintUsage();
    return 1;
  }
  const std::string& name = name_opt.value();
  const std::string& email = email_opt.value();

  user::v1::CreateUserRequest request;
  request.set_name(name);
  request.set_email(email);

  user::v1::CreateUserResponse response;
  grpc::ClientContext context;
  const grpc::Status status = stub->CreateUser(&context, request, &response);

  if (!status.ok()) {
    std::cout << "code=" << ExtractCodeFromGrpcStatus(status)
              << " message=" << status.error_message() << " user_id=0 created_at=" << std::endl;
    return 2;
  }

  std::cout << "code=" << response.code() << " message=" << response.message()
            << " user_id=" << response.user_id() << " created_at=" << response.created_at() << std::endl;
  return 0;
}

int RunGet(user::v1::UserService::Stub* stub, const std::vector<std::string>& args) {
  const auto user_id_opt = GetArgValue(args, "--user_id");
  if (!user_id_opt.has_value()) {
    PrintUsage();
    return 1;
  }
  const std::string& user_id_str = user_id_opt.value();
  if (user_id_str.empty()) {
    std::cout << "code=40001 message=invalid user_id" << std::endl;
    return 1;
  }

  int64_t user_id = 0;
  try {
    user_id = std::stoll(user_id_str);
  } catch (const std::exception&) {
    std::cout << "code=40001 message=invalid user_id" << std::endl;
    return 1;
  }

  user::v1::GetUserRequest request;
  request.set_user_id(user_id);

  user::v1::GetUserResponse response;
  grpc::ClientContext context;
  const grpc::Status status = stub->GetUser(&context, request, &response);

  if (!status.ok()) {
    std::cout << "code=" << ExtractCodeFromGrpcStatus(status)
              << " message=" << status.error_message() << std::endl;
    return 2;
  }

  std::cout << "code=" << response.code() << " message=" << response.message();
  if (response.code() == 0) {
    std::cout << " id=" << response.user().id()
              << " name=" << response.user().name()
              << " email=" << response.user().email()
              << " created_at=" << response.user().created_at();
  }
  std::cout << std::endl;

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  std::string target = "127.0.0.1:50051";
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  std::vector<std::string> normalized;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--target" && i + 1 < args.size()) {
      target = args[i + 1];
      ++i;
      continue;
    }
    normalized.push_back(args[i]);
  }

  if (normalized.empty()) {
    PrintUsage();
    return 1;
  }

  const std::string command = normalized[0];

  auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
  std::unique_ptr<user::v1::UserService::Stub> stub = user::v1::UserService::NewStub(channel);

  if (command == "create") {
    return RunCreate(stub.get(), normalized);
  }
  if (command == "get") {
    return RunGet(stub.get(), normalized);
  }

  PrintUsage();
  return 1;
}
