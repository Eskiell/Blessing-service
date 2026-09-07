#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "ezcheats/overlay/hold_shortcut.hpp"

namespace {

constexpr const char* kStateDirectory = "/system_tmp/blessing";
constexpr const char* kReadyPath = "/system_tmp/blessing/overlay-ready";
constexpr const char* kInputReadyPath =
    "/system_tmp/blessing/overlay-input-ready";
constexpr uint32_t kPadButtonL3 = 0x00000002;
constexpr uint32_t kPadButtonR3 = 0x00000004;
constexpr uint32_t kShortcutButtons = kPadButtonL3 | kPadButtonR3;
constexpr uint64_t kShortcutHoldMs = 1000;
constexpr useconds_t kPollDelayUs = 33 * 1000;
constexpr useconds_t kRetryDelayUs = 1000 * 1000;

extern "C" {
int sceUserServiceGetForegroundUser(int* user_id);
int scePadGetHandle(int user_id, int controller_type, int controller_index);
int scePadReadState(int handle, void* state);
int sceKernelDlsym(int handle, const char* name, void** address);
}

using SendNotification = int (*)(int, void*, size_t, int);

SendNotification g_send_notification = nullptr;

struct NotificationRequest {
  char reserved[45];
  char message[3075];
};

struct alignas(16) PadState {
  uint8_t bytes[256];

  uint32_t buttons() const noexcept {
    uint32_t value = 0;
    memcpy(&value, bytes, sizeof(value));
    return value;
  }
};

uint64_t monotonic_ms() noexcept {
  struct timespec time {};
  if (clock_gettime(CLOCK_MONOTONIC, &time) != 0) return 0;
  return static_cast<uint64_t>(time.tv_sec) * 1000 +
         static_cast<uint64_t>(time.tv_nsec) / 1000000;
}

bool write_pid_marker(const char* path) {
  const int file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (file < 0) return false;
  char value[32];
  const int length = snprintf(value, sizeof(value), "%d", getpid());
  const bool written = length > 0 &&
                       write(file, value, static_cast<size_t>(length)) == length;
  close(file);
  return written;
}

bool publish_ready() {
  mkdir(kStateDirectory, 0777);
  return write_pid_marker(kReadyPath);
}

void show_test_panel(bool visible) {
  printf("Blessing overlay: shortcut accepted panel=%s\n",
         visible ? "open" : "closed");
  if (g_send_notification == nullptr) {
    printf("Blessing overlay: notification unavailable; request skipped\n");
    return;
  }
  NotificationRequest request{};
  snprintf(request.message, sizeof(request.message),
           visible ? "Blessing\nOverlay de teste aberto\nNenhum cheat foi alterado."
                   : "Blessing\nOverlay de teste fechado");
  printf("Blessing overlay: notification dispatch begin\n");
  const int result = g_send_notification(0, &request, sizeof(request), 0);
  printf("Blessing overlay: test panel=%s notify=0x%x\n",
         visible ? "open" : "closed", result);
}

bool resolve_notification_sender() {
  constexpr int kLibKernelHandles[] = {1, 0x2001};
  for (const int handle : kLibKernelHandles) {
    void* address = nullptr;
    const int result = sceKernelDlsym(
        handle, "sceKernelSendNotificationRequest", &address);
    if (result == 0 && address != nullptr) {
      g_send_notification = reinterpret_cast<SendNotification>(address);
      printf("Blessing overlay: notification ready handle=0x%x address=%p\n",
             handle, address);
      return true;
    }
    printf("Blessing overlay: notification resolve handle=0x%x result=0x%x\n",
           handle, result);
  }
  return false;
}

int acquire_pad_handle() {
  int user_id = -1;
  if (sceUserServiceGetForegroundUser(&user_id) != 0 || user_id < 0) {
    return -1;
  }
  const int handle = scePadGetHandle(user_id, 0, 0);
  printf("Blessing overlay: pad user=%d handle=0x%x\n", user_id, handle);
  return handle;
}

void run_shortcut_loop() {
  ezcheats::overlay::HoldShortcut shortcut{kShortcutButtons, kShortcutHoldMs};
  bool panel_visible = false;
  int handle = -1;
  int read_failures = 0;

  for (;;) {
    if (handle < 0) {
      handle = acquire_pad_handle();
      if (handle < 0) {
        usleep(kRetryDelayUs);
        continue;
      }
      write_pid_marker(kInputReadyPath);
      printf("Blessing overlay: input ready; hold L3+R3 for 1 second\n");
    }

    PadState state{};
    const int result = scePadReadState(handle, &state);
    if (result != 0) {
      shortcut.reset();
      if (++read_failures >= 30) {
        printf("Blessing overlay: pad read failed=0x%x; reacquiring\n",
               result);
        read_failures = 0;
        handle = -1;
        unlink(kInputReadyPath);
      }
      usleep(kPollDelayUs);
      continue;
    }
    read_failures = 0;
    if (shortcut.update(state.buttons(), monotonic_ms()) ==
        ezcheats::overlay::ShortcutEvent::activated) {
      panel_visible = !panel_visible;
      show_test_panel(panel_visible);
    }
    usleep(kPollDelayUs);
  }
}

}  // namespace

int main() {
  const bool ready = publish_ready();
  printf("Blessing overlay: inert module loaded pid=%d ready=%s\n", getpid(),
         ready ? "yes" : "no");
  if (!resolve_notification_sender()) {
    printf("Blessing overlay: notification resolver unavailable; continuing safely\n");
  }
  run_shortcut_loop();
}
