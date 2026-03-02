#pragma once

#include <string>

namespace my_demo {

struct RequestContext {
  std::string trace_id;
  std::string peer;
  std::string method;
};

}  // namespace my_demo
