#include "ezcheats/http/http_server.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <stddef.h>

#include "ezcheats/assets/embedded_frontend.hpp"
#include "ezcheats/http/http_routes.hpp"
#include "ezcheats/platform/unique_fd.hpp"
#include "ezcheats/version.hpp"

namespace ezcheats::http {
namespace {

constexpr int kBacklog = 8;
constexpr size_t kRequestLimit = 8 * 1024;
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

void handle_client(int fd) {
  char request[kRequestLimit + 1]{};
  const auto count = ::recv(fd, request, kRequestLimit, 0);
  if (count <= 0) {
    return;
  }

  request[static_cast<size_t>(count)] = '\0';
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

HttpServer::HttpServer(uint16_t port) noexcept : port_(port) {}

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
    handle_client(client.get());
  }
}

}  // namespace ezcheats::http
