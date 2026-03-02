set(PROTO_ROOT "${CMAKE_SOURCE_DIR}/proto")
set(USER_PROTO_FILE "${PROTO_ROOT}/user_service.proto")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(MAKE_DIRECTORY "${GEN_DIR}")

set(USER_PB_CC   "${GEN_DIR}/user_service.pb.cc")
set(USER_PB_H    "${GEN_DIR}/user_service.pb.h")
set(USER_GRPC_CC "${GEN_DIR}/user_service.grpc.pb.cc")
set(USER_GRPC_H  "${GEN_DIR}/user_service.grpc.pb.h")

add_custom_command(
  OUTPUT "${USER_PB_CC}" "${USER_PB_H}" "${USER_GRPC_CC}" "${USER_GRPC_H}"
  COMMAND protobuf::protoc
  ARGS
    --proto_path "${PROTO_ROOT}"
    --cpp_out "${GEN_DIR}"
    --grpc_out "${GEN_DIR}"
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    "${USER_PROTO_FILE}"
  DEPENDS "${USER_PROTO_FILE}"
  VERBATIM
)

add_library(user_proto STATIC
  "${USER_PB_CC}"
  "${USER_GRPC_CC}"
)

target_include_directories(user_proto PUBLIC
  "${GEN_DIR}"
)

target_link_libraries(user_proto PUBLIC
  protobuf::libprotobuf
  gRPC::grpc++
)

add_library(user_service_lib STATIC
  src/handler/user_rpc_handler.cpp
  src/service/user_domain_service.cpp
  src/repository/user_repository.cpp
  src/cache/user_cache.cpp
  src/infra/mysql_client.cpp
  src/infra/redis_client.cpp
  src/infra/config_loader.cpp
  src/infra/logger.cpp
)

target_include_directories(user_service_lib PUBLIC
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${GEN_DIR}"
)

target_link_libraries(user_service_lib PUBLIC
  user_proto
  mysql::client
  hiredis::hiredis
  yaml-cpp::yaml-cpp
  spdlog::spdlog
  Threads::Threads
)

add_executable(user_service src/main.cpp)

target_link_libraries(user_service PRIVATE
  user_service_lib
)
