#include <cassert>

#include "ezcheats/application/in_memory_cheat_service.hpp"
#include "ezcheats/http/json_writer.hpp"
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
  assert(route("GET", "/api/v1/cheats") == Route::cheats);
  assert(route("PUT", "/api/v1/cheats/0") == Route::cheat_toggle);
  assert(route("PUT", "/api/v1/cheats/") == Route::method_not_allowed);
  assert(route("GET", "/missing") == Route::not_found);
  assert(route("POST", "/health") == Route::method_not_allowed);

  ezcheats::application::InMemoryCheatService service;
  auto state = service.state();
  assert(state.connected);
  assert(state.game != nullptr);
  assert(state.cheat_count == 3);
  assert(!state.cheats[0].enabled);

  ezcheats::domain::CheatEntry updated{};
  assert(service.set_enabled(0, true, updated));
  assert(updated.id == 0);
  assert(updated.enabled);
  assert(service.state().cheats[0].enabled);
  assert(!service.set_enabled(99, true, updated));

  char json_buffer[64]{};
  ezcheats::http::JsonWriter json{json_buffer, sizeof(json_buffer)};
  assert(json.append("{\"text\":"));
  assert(json.quoted("line\n\"quoted\""));
  assert(json.append(",\"enabled\":"));
  assert(json.boolean(true));
  assert(json.append("}"));
  assert(json.ok());
  assert(strcmp(json.data(),
                "{\"text\":\"line\\n\\\"quoted\\\"\",\"enabled\":true}") == 0);

  char tiny_buffer[2]{};
  ezcheats::http::JsonWriter tiny{tiny_buffer, sizeof(tiny_buffer)};
  assert(!tiny.append("too large"));
  assert(!tiny.ok());
}
