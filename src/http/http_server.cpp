#include "ezcheats/http/http_server.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <stddef.h>

#include "ezcheats/assets/embedded_frontend.hpp"
#include "ezcheats/http/http_routes.hpp"
#include "ezcheats/http/json_writer.hpp"
#include "ezcheats/platform/unique_fd.hpp"
#include "ezcheats/version.hpp"

namespace ezcheats::http {
namespace {

constexpr int kBacklog = 8;
constexpr size_t kRequestLimit = 8 * 1024;
constexpr size_t kResponseLimit = 48 * 1024;
constexpr const char* kHealth = R"({"status":"ok"})";
constexpr const char* kVersion =
    R"({"name":"ez-cheats","version":"0.1.0","apiVersion":1})";
constexpr const char* kNotFound = R"({"error":"not_found"})";
constexpr const char* kMethodNotAllowed = R"({"error":"method_not_allowed"})";
constexpr const char* kBadRequest = R"({"error":"bad_request"})";

bool send_all(int fd, const void* data, size_t size) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  size_t sent = 0;
  while (sent < size) {
    const auto count = ::send(fd, bytes + sent, size - sent, 0);
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count <= 0) {
      return false;
    }
    sent += static_cast<size_t>(count);
  }
  return true;
}

void respond(int fd, int status, const char* reason, const char* content_type,
             const void* body, size_t body_size,
             const char* extra_headers = "") {
  char header[512]{};
  const int header_size = snprintf(
      header, sizeof(header),
      "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
      "Cache-Control: no-store\r\nConnection: close\r\n%.*s\r\n",
      status, reason, content_type, body_size, (int)strlen(extra_headers),
      extra_headers);
  if (header_size <= 0 || static_cast<size_t>(header_size) >= sizeof(header)) {
    return;
  }
  if (!send_all(fd, header, static_cast<size_t>(header_size))) {
    return;
  }
  if (body_size > 0) {
    send_all(fd, body, body_size);
  }
}

void respond_json(int fd, int status, const char* reason, const char* body,
                  const char* extra_headers = "") {
  respond(fd, status, reason, "application/json; charset=utf-8", body,
          strlen(body), extra_headers);
}

bool parse_request_line(char* request, char*& method, char*& target) {
  char* line_end = strstr(request, "\r\n");
  if (line_end == nullptr) {
    return false;
  }
  *line_end = '\0';
  char* first_space = strchr(request, ' ');
  char* second_space = first_space == nullptr ? nullptr : strchr(first_space + 1, ' ');
  if (first_space == request || second_space == nullptr ||
      second_space == first_space + 1 || strcmp(second_space + 1, "HTTP/1.1") != 0) {
    return false;
  }
  *first_space = '\0';
  *second_space = '\0';
  method = request;
  target = first_space + 1;
  return true;
}

const char* find_header(const char* headers, const char* name) {
  const size_t name_size = strlen(name);
  for (const char* line = headers; *line != '\0'; ++line) {
    size_t i = 0;
    while (i < name_size && line[i] != '\0') {
      char a = line[i];
      char b = name[i];
      if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
      if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
      if (a != b) break;
      ++i;
    }
    if (i == name_size && line[i] == ':') return line + i + 1;
  }
  return nullptr;
}

bool read_request(int fd, char* request, size_t capacity, char*& body) {
  size_t received = 0;
  char* header_end = nullptr;
  size_t expected = 0;
  while (received + 1 < capacity) {
    const auto count = ::recv(fd, request + received, capacity - received - 1, 0);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) return false;
    received += static_cast<size_t>(count);
    request[received] = '\0';

    if (header_end == nullptr) {
      header_end = strstr(request, "\r\n\r\n");
      if (header_end == nullptr) continue;
      body = header_end + 4;
      const char* length = find_header(request, "Content-Length");
      if (length != nullptr) expected = static_cast<size_t>(strtoul(length, nullptr, 10));
      if (expected > capacity - static_cast<size_t>(body - request) - 1) return false;
    }
    if (received >= static_cast<size_t>(body - request) + expected) return true;
  }
  return false;
}

bool parse_toggle(const char* target, const char* body, uint32_t& id,
                  bool& enabled) {
  constexpr const char* prefix = "/api/v1/cheats/";
  char* end = nullptr;
  const unsigned long parsed = strtoul(target + strlen(prefix), &end, 10);
  if (end == target + strlen(prefix) || (*end != '\0' && *end != '?') ||
      parsed > UINT32_MAX) {
    return false;
  }
  const char* field = strstr(body, "\"enabled\"");
  if (field == nullptr || (field = strchr(field, ':')) == nullptr) return false;
  ++field;
  while (*field == ' ' || *field == '\t' || *field == '\r' || *field == '\n') ++field;
  if (strncmp(field, "true", 4) == 0) {
    enabled = true;
  } else if (strncmp(field, "false", 5) == 0) {
    enabled = false;
  } else {
    return false;
  }
  id = static_cast<uint32_t>(parsed);
  return true;
}

bool write_cheat(JsonWriter& json, const domain::CheatEntry& cheat) {
  return json.append("{\"id\":") && json.number(cheat.id) &&
         json.append(",\"name\":") && json.quoted(cheat.name) &&
         json.append(",\"description\":") && json.quoted(cheat.description) &&
         json.append(",\"author\":") && json.quoted(cheat.author) &&
         json.append(",\"enabled\":") && json.boolean(cheat.enabled) &&
         json.append("}");
}

bool write_state(JsonWriter& json, const domain::ServiceState& state) {
  if (!json.append("{\"connected\":") || !json.boolean(state.connected) ||
      !json.append(",\"backend\":") || !json.quoted(state.backend)) return false;
  if (state.game == nullptr) {
    if (!json.append(",\"game\":null")) return false;
  } else if (!json.append(",\"game\":{\"titleId\":") ||
             !json.quoted(state.game->title_id) || !json.append(",\"name\":") ||
             !json.quoted(state.game->name) || !json.append(",\"version\":") ||
             !json.quoted(state.game->version) || !json.append(",\"platform\":") ||
             !json.quoted(state.game->platform) || !json.append("}")) {
    return false;
  }
  if (!json.append(",\"cheats\":[")) return false;
  for (size_t i = 0; i < state.cheat_count; ++i) {
    if ((i > 0 && !json.append(",")) || !write_cheat(json, state.cheats[i])) return false;
  }
  return json.append("]}");
}

void handle_client(int fd, domain::ICheatService& cheat_service) {
  char request[kRequestLimit + 1]{};
  char* body = nullptr;
  if (!read_request(fd, request, sizeof(request), body)) return;
  char* method = nullptr;
  char* target = nullptr;
  if (!parse_request_line(request, method, target)) {
    respond_json(fd, 400, "Bad Request", kBadRequest);
    return;
  }

  switch (route(method, target)) {
    case Route::frontend: {
      const auto asset = assets::frontend();
      respond(fd, 200, "OK", "text/html; charset=utf-8", asset.data,
              asset.size);
      break;
    }
    case Route::health:
      respond_json(fd, 200, "OK", kHealth);
      break;
    case Route::version:
      respond_json(fd, 200, "OK", kVersion);
      break;
    case Route::cheats: {
      char response[kResponseLimit]{};
      JsonWriter json{response, sizeof(response)};
      if (!write_state(json, cheat_service.state())) {
        respond_json(fd, 500, "Internal Server Error", R"({"error":"response_too_large"})");
      } else {
        respond_json(fd, 200, "OK", json.data());
      }
      break;
    }
    case Route::cheat_toggle: {
      uint32_t id = 0;
      bool enabled = false;
      if (!parse_toggle(target, body, id, enabled)) {
        respond_json(fd, 400, "Bad Request", kBadRequest);
        break;
      }
      domain::CheatEntry updated{};
      if (!cheat_service.set_enabled(id, enabled, updated)) {
        respond_json(fd, 404, "Not Found", kNotFound);
        break;
      }
      char response[1024]{};
      JsonWriter json{response, sizeof(response)};
      if (!write_cheat(json, updated)) {
        respond_json(fd, 500, "Internal Server Error", R"({"error":"response_too_large"})");
      } else {
        respond_json(fd, 200, "OK", json.data());
      }
      break;
    }
    case Route::method_not_allowed:
      respond_json(fd, 405, "Method Not Allowed", kMethodNotAllowed,
                   "Allow: GET\r\n");
      break;
    case Route::not_found:
      respond_json(fd, 404, "Not Found", kNotFound);
      break;
  }
}

}  // namespace

HttpServer::HttpServer(uint16_t port,
                       domain::ICheatService& cheat_service) noexcept
    : port_(port), cheat_service_(cheat_service) {}

int HttpServer::run() {
  platform::UniqueFd server{::socket(AF_INET, SOCK_STREAM, 0)};
  if (!server) {
    perror("socket");
    return 1;
  }

  int reuse = 1;
  if (::setsockopt(server.get(), SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) != 0) {
    perror("setsockopt");
    return 1;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port_);
  if (::bind(server.get(), reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) != 0) {
    perror("bind");
    return 1;
  }
  if (::listen(server.get(), kBacklog) != 0) {
    perror("listen");
    return 1;
  }

  printf("EZ Cheats listening on port %u\n", port_);
  for (;;) {
    platform::UniqueFd client{::accept(server.get(), nullptr, nullptr)};
    if (!client) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      return 1;
    }
    handle_client(client.get(), cheat_service_);
  }
}

}  // namespace ezcheats::http
