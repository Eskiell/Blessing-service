#include <stddef.h>
#include <string.h>

typedef struct notify_request {
  char reserved[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);

int
main(void) {
  notify_request_t request;
  memset(&request, 0, sizeof(request));
  strncpy(request.message, "EZHELIT: ELF executado com sucesso",
          sizeof(request.message) - 1);
  return sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}
