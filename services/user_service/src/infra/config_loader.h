#pragma once

#include <string>

#include "common/base/app_result.h"
#include "services/user_service/src/infra/config_types.h"

namespace my_demo {

class ConfigLoader {
 public:
  static AppResult<UserServiceConfig> Load(const std::string& file_path);
};

}  // namespace my_demo
