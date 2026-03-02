add_executable(user_client
  main.cpp
)

target_include_directories(user_client PRIVATE
  "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(user_client PRIVATE
  user_proto
  gRPC::grpc++
  protobuf::libprotobuf
)
