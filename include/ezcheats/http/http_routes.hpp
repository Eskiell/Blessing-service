#pragma once

#include <string.h>

namespace ezcheats::http {

enum class Route {
  frontend,
  health,
  version,
  not_found,
  method_not_allowed,
};

inline bool target_is(const char* target, const char* expected) {
  const size_t length = strlen(expected);
  return strncmp(target, expected, length) == 0 &&
         (target[length] == '\0' || target[length] == '?');
}

inline Route route(const char* method, const char* target) {
  if (strcmp(method, "GET") != 0) {
    return Route::method_not_allowed;
  }
  if (target_is(target, "/") || target_is(target, "/index.html")) {
    return Route::frontend;
  }
  if (target_is(target, "/health")) {
    return Route::health;
  }
  if (target_is(target, "/api/v1/version")) {
    return Route::version;
  }
  return Route::not_found;
}

}  // namespace ezcheats::http
