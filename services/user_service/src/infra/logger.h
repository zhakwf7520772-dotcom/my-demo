#pragma once

#include <memory>

#include <spdlog/logger.h>

#include "common/base/app_result.h"
#include "services/user_service/src/infra/config_types.h"

namespace my_demo {

AppResult<void> InitLogger(const LogConfig& config);
std::shared_ptr<spdlog::logger> GetLogger();

}  // namespace my_demo
