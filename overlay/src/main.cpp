#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr const char* kStateDirectory = "/system_tmp/blessing";
constexpr const char* kReadyPath = "/system_tmp/blessing/overlay-ready";

bool publish_ready() {
  mkdir(kStateDirectory, 0777);
  const int file = open(kReadyPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (file < 0) return false;
  char value[32];
  const int length = snprintf(value, sizeof(value), "%d", getpid());
  const bool written = length > 0 &&
                       write(file, value, static_cast<size_t>(length)) == length;
  close(file);
  return written;
}

}  // namespace

int main() {
  const bool ready = publish_ready();
  printf("Blessing overlay: inert module loaded pid=%d ready=%s\n", getpid(),
         ready ? "yes" : "no");

  // PR 23 deliberately installs no hooks. Keep the injected thread alive so
  // later PRs can attach ShellUI-owned state without changing the loader ABI.
  for (;;) sleep(60 * 60);
}
