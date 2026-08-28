#include <cassert>

#include "ezcheats/http/http_routes.hpp"

int main() {
  using ezcheats::http::Route;
  using ezcheats::http::route;

  assert(route("GET", "/") == Route::frontend);
  assert(route("GET", "/index.html") == Route::frontend);
  assert(route("GET", "/health") == Route::health);
  assert(route("GET", "/health?nonce=1") == Route::health);
  assert(route("GET", "/healthcheck") == Route::not_found);
  assert(route("GET", "/api/v1/version") == Route::version);
  assert(route("GET", "/api/v1/version/extra") == Route::not_found);
  assert(route("GET", "/missing") == Route::not_found);
  assert(route("POST", "/health") == Route::method_not_allowed);
}
