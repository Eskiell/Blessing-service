/*
 * EZHELIT Store - minimal persistent download experiment.
 *
 * UI:       http://<PS5-IP>:5911/
 * Source:   http://192.168.15.125:8080/ezhelit.exfat.png
 * Output:   /data/ezhelit-store/downloads/ezhelit.exfat.png
 *
 * The HTTP server and download worker live in the payload. Closing the web UI
 * does not stop the worker while this payload remains running.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_installer.h"

#define LISTEN_PORT 5911
#define BACKLOG 8
#define REQUEST_SIZE 4096
#define IO_BUFFER_SIZE 65536

#define DOWNLOAD_HOST "192.168.15.125"
#define DOWNLOAD_PORT 8080
#define DOWNLOAD_PATH "/ezhelit.exfat.png"
#define DOWNLOAD_URL "http://192.168.15.125:8080/ezhelit.exfat.png"

#define DATA_DIR "/data/ezhelit-store"
#define DOWNLOAD_DIR DATA_DIR "/downloads"
#define PART_PATH DOWNLOAD_DIR "/ezhelit.exfat.png.part"
#define FINAL_PATH DOWNLOAD_DIR "/ezhelit.exfat.png"

#ifndef UI_ASSET_PATH
#define UI_ASSET_PATH "frontend/dist/index.html"
#endif

#define INCASSET(name, file)                                                  \
  __asm__(".section .rodata\n"                                                \
          ".global " #name "\n"                                               \
          ".global " #name "_end\n"                                           \
          ".global " #name "_size\n"                                          \
          ".align 16\n" #name ":\n"                                           \
          ".incbin \"" file "\"\n" #name "_end:\n" #name "_size:\n"           \
          ".quad " #name "_end - " #name "\n"                                 \
          ".previous\n");                                                     \
  extern const uint8_t name[];                                                \
  extern const size_t name##_size

INCASSET(frontend_index, UI_ASSET_PATH);

typedef enum download_state {
  DOWNLOAD_IDLE,
  DOWNLOAD_CONNECTING,
  DOWNLOAD_DOWNLOADING,
  DOWNLOAD_COMPLETED,
  DOWNLOAD_FAILED
} download_state_t;

typedef struct download_status {
  download_state_t state;
  uint64_t received;
  uint64_t total;
  char error[160];
} download_status_t;

typedef struct notify_request {
  char reserved[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);

static pthread_mutex_t g_status_lock = PTHREAD_MUTEX_INITIALIZER;
static download_status_t g_status = {DOWNLOAD_IDLE, 0, 0, ""};

static void
notify(const char *message) {
  notify_request_t request;
  memset(&request, 0, sizeof(request));
  snprintf(request.message, sizeof(request.message), "%s", message);
  sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

static void
set_status(download_state_t state, uint64_t received, uint64_t total,
           const char *error) {
  pthread_mutex_lock(&g_status_lock);
  g_status.state = state;
  g_status.received = received;
  g_status.total = total;
  snprintf(g_status.error, sizeof(g_status.error), "%s", error ? error : "");
  pthread_mutex_unlock(&g_status_lock);
}

static void
set_error(const char *format, ...) {
  char message[160];
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);
  set_status(DOWNLOAD_FAILED, 0, 0, message);
}

static int
mkdir_if_needed(const char *path) {
  if(mkdir(path, 0755) == 0 || errno == EEXIST) return 0;
  return -1;
}

static int
send_all(int fd, const void *data, size_t size) {
  const uint8_t *bytes = data;
  size_t sent = 0;
  while(sent < size) {
    ssize_t result = send(fd, bytes + sent, size - sent, 0);
    if(result <= 0) return -1;
    sent += (size_t)result;
  }
  return 0;
}

static int
parse_content_length(const char *headers, uint64_t *value) {
  const char *line = strcasestr(headers, "Content-Length:");
  if(!line) return -1;
  line += strlen("Content-Length:");
  while(*line == ' ' || *line == '\t') line++;
  *value = strtoull(line, NULL, 10);
  return *value > 0 ? 0 : -1;
}

static void *
download_worker(void *unused) {
  (void)unused;
  int socket_fd = -1;
  FILE *output = NULL;
  uint8_t *buffer = NULL;
  uint64_t received = 0;
  uint64_t total = 0;

  set_status(DOWNLOAD_CONNECTING, 0, 0, NULL);

  if(mkdir_if_needed(DATA_DIR) != 0 || mkdir_if_needed(DOWNLOAD_DIR) != 0) {
    set_error("Nao foi possivel criar %s", DOWNLOAD_DIR);
    return NULL;
  }

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(socket_fd < 0) {
    set_error("Falha ao criar socket");
    return NULL;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(DOWNLOAD_PORT);
  if(inet_pton(AF_INET, DOWNLOAD_HOST, &address.sin_addr) != 1 ||
     connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
    set_error("Nao foi possivel conectar a %s:%d", DOWNLOAD_HOST,
              DOWNLOAD_PORT);
    close(socket_fd);
    return NULL;
  }

  char request[512];
  int request_size = snprintf(
      request, sizeof(request),
      "GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\n\r\n",
      DOWNLOAD_PATH, DOWNLOAD_HOST, DOWNLOAD_PORT);
  if(send_all(socket_fd, request, (size_t)request_size) != 0) {
    set_error("Falha ao enviar requisicao");
    close(socket_fd);
    return NULL;
  }

  buffer = malloc(IO_BUFFER_SIZE + 1);
  if(!buffer) {
    set_error("Sem memoria para buffer");
    close(socket_fd);
    return NULL;
  }

  size_t buffered = 0;
  char *header_end = NULL;
  while(buffered < IO_BUFFER_SIZE) {
    ssize_t count = recv(socket_fd, buffer + buffered,
                         IO_BUFFER_SIZE - buffered, 0);
    if(count <= 0) break;
    buffered += (size_t)count;
    buffer[buffered] = '\0';
    header_end = strstr((char *)buffer, "\r\n\r\n");
    if(header_end) break;
  }

  if(!header_end) {
    set_error("Resposta HTTP invalida");
    goto cleanup;
  }
  if(strncmp((char *)buffer, "HTTP/1.1 200", 12) != 0 &&
     strncmp((char *)buffer, "HTTP/1.0 200", 12) != 0) {
    char status[48] = {0};
    sscanf((char *)buffer, "%47[^\r\n]", status);
    set_error("Servidor respondeu: %s", status);
    goto cleanup;
  }
  if(parse_content_length((char *)buffer, &total) != 0) {
    set_error("Servidor nao informou Content-Length");
    goto cleanup;
  }

  output = fopen(PART_PATH, "wb");
  if(!output) {
    set_error("Nao foi possivel abrir o destino temporario");
    goto cleanup;
  }

  size_t header_size = (size_t)(header_end + 4 - (char *)buffer);
  size_t body_size = buffered - header_size;
  if(body_size > 0) {
    if(fwrite(buffer + header_size, 1, body_size, output) != body_size) {
      set_error("Falha ao gravar arquivo");
      goto cleanup;
    }
    received += body_size;
  }
  set_status(DOWNLOAD_DOWNLOADING, received, total, NULL);

  for(;;) {
    ssize_t count = recv(socket_fd, buffer, IO_BUFFER_SIZE, 0);
    if(count == 0) break;
    if(count < 0) {
      set_error("Conexao interrompida");
      goto cleanup;
    }
    if(fwrite(buffer, 1, (size_t)count, output) != (size_t)count) {
      set_error("Falha ao gravar arquivo");
      goto cleanup;
    }
    received += (uint64_t)count;
    set_status(DOWNLOAD_DOWNLOADING, received, total, NULL);
  }

  if(received != total) {
    set_error("Tamanho incorreto: %llu de %llu bytes",
              (unsigned long long)received, (unsigned long long)total);
    goto cleanup;
  }

  if(fclose(output) != 0) {
    output = NULL;
    set_error("Falha ao finalizar arquivo");
    goto cleanup;
  }
  output = NULL;

  unlink(FINAL_PATH);
  if(rename(PART_PATH, FINAL_PATH) != 0) {
    set_error("Falha ao renomear arquivo concluido");
    goto cleanup;
  }

  set_status(DOWNLOAD_COMPLETED, received, total, NULL);

cleanup:
  if(output) fclose(output);
  if(socket_fd >= 0) close(socket_fd);
  free(buffer);
  return NULL;
}

static int
start_download(void) {
  pthread_mutex_lock(&g_status_lock);
  int busy = g_status.state == DOWNLOAD_CONNECTING ||
             g_status.state == DOWNLOAD_DOWNLOADING;
  pthread_mutex_unlock(&g_status_lock);
  if(busy) return 0;

  set_status(DOWNLOAD_CONNECTING, 0, 0, NULL);
  pthread_t thread;
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  int result = pthread_create(&thread, &attributes, download_worker, NULL);
  pthread_attr_destroy(&attributes);
  if(result != 0) {
    set_error("Falha ao iniciar worker: %d", result);
    return -1;
  }
  return 0;
}

static const char *
state_name(download_state_t state) {
  switch(state) {
    case DOWNLOAD_CONNECTING: return "connecting";
    case DOWNLOAD_DOWNLOADING: return "downloading";
    case DOWNLOAD_COMPLETED: return "completed";
    case DOWNLOAD_FAILED: return "failed";
    default: return "idle";
  }
}

static const char *
state_label(download_state_t state) {
  switch(state) {
    case DOWNLOAD_CONNECTING: return "Conectando...";
    case DOWNLOAD_DOWNLOADING: return "Baixando...";
    case DOWNLOAD_COMPLETED: return "Download concluido";
    case DOWNLOAD_FAILED: return "Download falhou";
    default: return "Aguardando";
  }
}

static void
respond_data(int client_fd, int status, const char *content_type,
             const void *body, size_t body_size) {
  char header[512];
  int header_size = snprintf(
      header, sizeof(header),
      "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
      "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
      status, status == 200 ? "OK" : "Error", content_type, body_size);
  send_all(client_fd, header, (size_t)header_size);
  send_all(client_fd, body, body_size);
}

static void
respond(int client_fd, int status, const char *content_type,
        const char *body) {
  respond_data(client_fd, status, content_type, body, strlen(body));
}

static void
handle_client(int client_fd) {
  char request[REQUEST_SIZE];
  ssize_t size = recv(client_fd, request, sizeof(request) - 1, 0);
  if(size <= 0) {
    close(client_fd);
    return;
  }
  request[size] = '\0';

  if(strncmp(request, "GET /api/v1/test-download/status ", 33) == 0) {
    download_status_t snapshot;
    pthread_mutex_lock(&g_status_lock);
    snapshot = g_status;
    pthread_mutex_unlock(&g_status_lock);

    char body[512];
    snprintf(body, sizeof(body),
             "{\"state\":\"%s\",\"label\":\"%s\","
             "\"receivedBytes\":%llu,\"totalBytes\":%llu,"
             "\"sourceUrl\":\"%s\",\"destination\":\"%s\","
             "\"error\":\"%s\"}",
             state_name(snapshot.state), state_label(snapshot.state),
             (unsigned long long)snapshot.received,
             (unsigned long long)snapshot.total, DOWNLOAD_URL, FINAL_PATH,
             snapshot.error);
    respond(client_fd, 200, "application/json; charset=utf-8", body);
  } else if(strncmp(request, "POST /api/v1/test-download ", 27) == 0) {
    start_download();
    respond(client_fd, 200, "application/json; charset=utf-8",
            "{\"accepted\":true}");
  } else if(strncmp(request, "GET /health ", 12) == 0) {
    respond(client_fd, 200, "application/json; charset=utf-8",
            "{\"status\":\"ok\"}");
  } else if(strncmp(request, "GET / ", 6) == 0) {
    respond_data(client_fd, 200, "text/html; charset=utf-8", frontend_index,
                 frontend_index_size);
  } else {
    respond(client_fd, 404, "application/json; charset=utf-8",
            "{\"error\":\"not_found\"}");
  }
  close(client_fd);
}

int
main(void) {
  /*
   * The launcher is now part of this payload's startup. Installation failure
   * is non-fatal: the server remains usable through the PS5 IP and port 5911.
   */
  app_install_if_needed();

  signal(SIGPIPE, SIG_IGN);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd < 0) return 1;

  int reuse = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(LISTEN_PORT);

  if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) != 0 ||
     listen(server_fd, BACKLOG) != 0) {
    notify("EZHELIT Store: porta 5911 indisponivel");
    close(server_fd);
    return 1;
  }

  notify("EZHELIT Store pronta em http://127.0.0.1:5911/");

  for(;;) {
    int client_fd = accept(server_fd, NULL, NULL);
    if(client_fd >= 0) handle_client(client_fd);
  }
}
