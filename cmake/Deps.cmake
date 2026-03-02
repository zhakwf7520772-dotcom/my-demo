include(FetchContent)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Locate MySQL C client headers/libs. Prefer BT panel path, then system paths.
find_path(MYSQL_INCLUDE_DIR
  NAMES mysql.h mysql/mysql.h
  PATHS
    /www/server/mysql/include
    /www/server/mysql
    /usr/include
    /usr/local/include
  PATH_SUFFIXES
    include
    mysql
    mariadb
)

find_library(MYSQL_LIBRARY
  NAMES mysqlclient
  PATHS
    /www/server/mysql/lib
    /usr/lib64/mysql
    /usr/lib/x86_64-linux-gnu
    /usr/lib64
    /usr/lib
    /lib64
    /lib
)

if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARY)
  message(FATAL_ERROR "MySQL client dev files not found. MYSQL_INCLUDE_DIR='${MYSQL_INCLUDE_DIR}', MYSQL_LIBRARY='${MYSQL_LIBRARY}'")
endif()

add_library(mysql::client UNKNOWN IMPORTED)
set_target_properties(mysql::client PROPERTIES
  IMPORTED_LOCATION "${MYSQL_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# hiredis
FetchContent_Declare(
  hiredis
  GIT_REPOSITORY https://github.com/redis/hiredis.git
  GIT_TAG v1.2.0
)
set(DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(hiredis)
if(TARGET hiredis AND NOT TARGET hiredis::hiredis)
  add_library(hiredis::hiredis ALIAS hiredis)
endif()
if(NOT TARGET hiredis::hiredis)
  message(FATAL_ERROR "Failed to resolve hiredis::hiredis target")
endif()

# yaml-cpp
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.7.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)
if(TARGET yaml-cpp AND NOT TARGET yaml-cpp::yaml-cpp)
  add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1
)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)
