#include "services/user_service/src/infra/logger.h"

#include <mutex>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "common/errors/error_code.h"

namespace my_demo {
namespace {

std::shared_ptr<spdlog::logger> g_logger;
std::once_flag g_init_flag;

spdlog::level::level_enum ParseLevel(const std::string& level) {
  if (level == "trace") return spdlog::level::trace;
  if (level == "debug") return spdlog::level::debug;
  if (level == "warn") return spdlog::level::warn;
  if (level == "error") return spdlog::level::err;
  if (level == "critical") return spdlog::level::critical;
  return spdlog::level::info;
}

}  // namespace

AppResult<void> InitLogger(const LogConfig& config) {
  try {
    std::call_once(g_init_flag, [&config]() {
      if (!config.log_path.empty()) {
        g_logger = spdlog::basic_logger_mt("user_service", config.log_path, true);
      } else {
        g_logger = spdlog::stdout_color_mt("user_service");
      }
      g_logger->set_level(ParseLevel(config.level));
      g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
      spdlog::set_default_logger(g_logger);
      spdlog::flush_on(spdlog::level::info);
    });
  } catch (const std::exception& ex) {
    return AppResult<void>::Err(
        ErrorCode::kInvalidArgument,
        std::string("init logger failed: ") + ex.what());
  }

  return AppResult<void>::Ok();
}

std::shared_ptr<spdlog::logger> GetLogger() {
  if (g_logger == nullptr) {
    g_logger = spdlog::default_logger();
    if (g_logger == nullptr) {
      g_logger = spdlog::stdout_color_mt("user_service_fallback");
      spdlog::set_default_logger(g_logger);
    }
  }
  return g_logger;
}

}  // namespace my_demo
