#include "ezcheats/platform/ps5_media_tile.hpp"

#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <ps5/kernel.h>

#ifndef EZ_CHEATS_MEDIA_PARAM_PATH
#error "EZ_CHEATS_MEDIA_PARAM_PATH must point to installer/param.json"
#endif
#ifndef EZ_CHEATS_MEDIA_ICON_PATH
#error "EZ_CHEATS_MEDIA_ICON_PATH must point to installer/icon0.png"
#endif

#define EMBED_ASSET(name, file)                                               \
  __asm__(".section .rodata\n"                                               \
          ".global " #name "\n"                                             \
          ".global " #name "_end\n"                                         \
          ".global " #name "_size\n"                                        \
          ".align 16\n" #name ":\n"                                         \
          ".incbin \"" file "\"\n" #name "_end:\n" #name "_size:\n"        \
          ".quad " #name "_end - " #name "\n"                              \
          ".previous\n");

EMBED_ASSET(ez_cheats_media_param, EZ_CHEATS_MEDIA_PARAM_PATH)
EMBED_ASSET(ez_cheats_media_icon, EZ_CHEATS_MEDIA_ICON_PATH)

extern "C" {
extern const uint8_t ez_cheats_media_param[];
extern const size_t ez_cheats_media_param_size;
extern const uint8_t ez_cheats_media_icon[];
extern const size_t ez_cheats_media_icon_size;

int sceNetCtlInit();
int sceUserServiceInitialize(void*);
int sceAppInstUtilInitialize();
int sceAppInstUtilTerminate();
int sceAppInstUtilAppInstallAll(void*);
}

namespace ezcheats::platform {
namespace {

constexpr char kTitleId[] = "EZCH00001";
constexpr char kAppDirectory[] = "/user/app/EZCH00001";
constexpr char kSystemDirectory[] = "/user/app/EZCH00001/sce_sys";
constexpr char kParamPath[] = "/user/app/EZCH00001/sce_sys/param.json";
constexpr char kIconPath[] = "/user/app/EZCH00001/sce_sys/icon0.png";

static_assert(sizeof(kTitleId) == 10);
static_assert(kTitleId[0] >= 'A' && kTitleId[0] <= 'Z' &&
              kTitleId[1] >= 'A' && kTitleId[1] <= 'Z' &&
              kTitleId[2] >= 'A' && kTitleId[2] <= 'Z' &&
              kTitleId[3] >= 'A' && kTitleId[3] <= 'Z' &&
              kTitleId[4] >= '0' && kTitleId[4] <= '9' &&
              kTitleId[5] >= '0' && kTitleId[5] <= '9' &&
              kTitleId[6] >= '0' && kTitleId[6] <= '9' &&
              kTitleId[7] >= '0' && kTitleId[7] <= '9' &&
              kTitleId[8] >= '0' && kTitleId[8] <= '9');

struct NotificationRequest {
  char reserved[45];
  char message[3075];
};

static_assert(sizeof(NotificationRequest) == 0xc30);

extern "C" int sceKernelSendNotificationRequest(int, NotificationRequest*,
                                                 size_t, int);

void notify_tile(const char* format, ...) noexcept {
  NotificationRequest request{};
  va_list arguments;
  va_start(arguments, format);
  ::vsnprintf(request.message, sizeof(request.message), format, arguments);
  va_end(arguments);
  sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

bool file_matches(const char* path, const uint8_t* expected,
                  size_t expected_size) noexcept {
  struct stat info {};
  if (::stat(path, &info) != 0 || info.st_size < 0 ||
      static_cast<size_t>(info.st_size) != expected_size) {
    return false;
  }

  FILE* file = ::fopen(path, "rb");
  if (file == nullptr) return false;

  auto* bytes = static_cast<uint8_t*>(::malloc(expected_size));
  if (bytes == nullptr) {
    ::fclose(file);
    return false;
  }

  const size_t read_size = ::fread(bytes, 1, expected_size, file);
  const bool matches =
      read_size == expected_size && ::memcmp(bytes, expected, expected_size) == 0;
  ::free(bytes);
  ::fclose(file);
  return matches;
}

bool write_file(const char* path, const uint8_t* data, size_t size) noexcept {
  FILE* file = ::fopen(path, "wb");
  if (file == nullptr) return false;
  const bool written = ::fwrite(data, 1, size, file) == size;
  const bool closed = ::fclose(file) == 0;
  return written && closed;
}

int register_title() noexcept {
  using InstallTitleDirectory = int (*)(const char*, const char*, void*);
  InstallTitleDirectory install_title_directory = nullptr;
  uint32_t handle = 0;
  if (kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle) == 0) {
    install_title_directory = reinterpret_cast<InstallTitleDirectory>(
        kernel_dynlib_resolve(-1, handle, "Wudg3Xe3heE"));
  }
  if (install_title_directory != nullptr) {
    return install_title_directory(kTitleId, "/user/app/", nullptr);
  }
  return sceAppInstUtilAppInstallAll(nullptr);
}

}  // namespace

MediaTileResult install_media_tile_if_needed() noexcept {
  const bool assets_current =
      file_matches(kParamPath, ez_cheats_media_param,
                   ez_cheats_media_param_size) &&
      file_matches(kIconPath, ez_cheats_media_icon,
                   ez_cheats_media_icon_size);

  const int netctl_result = sceNetCtlInit();
  int user_priority = 256;
  const int user_result = sceUserServiceInitialize(&user_priority);
  const int initialize_result = sceAppInstUtilInitialize();
  printf("EZ Cheats: media tile init netctl=0x%08x user=0x%08x "
         "appinst=0x%08x\n",
         static_cast<uint32_t>(netctl_result),
         static_cast<uint32_t>(user_result),
         static_cast<uint32_t>(initialize_result));
  if (initialize_result != 0) {
    notify_tile("EZ Cheats - tile FALHOU\nAppInstUtil: 0x%08x",
                static_cast<uint32_t>(initialize_result));
    return MediaTileResult::failed;
  }

  if (!assets_current) {
    const bool directories_ready =
        (::mkdir(kAppDirectory, 0755) == 0 || errno == EEXIST) &&
        (::mkdir(kSystemDirectory, 0755) == 0 || errno == EEXIST);
    if (!directories_ready ||
        !write_file(kParamPath, ez_cheats_media_param,
                    ez_cheats_media_param_size) ||
        !write_file(kIconPath, ez_cheats_media_icon,
                    ez_cheats_media_icon_size)) {
      printf("EZ Cheats: media tile asset update failed errno=%d\n", errno);
      notify_tile("EZ Cheats - tile FALHOU\nAssets: errno %d", errno);
      sceAppInstUtilTerminate();
      return MediaTileResult::failed;
    }
  }

  const int result = register_title();
  printf("EZ Cheats: media tile register title=%s result=0x%08x\n", kTitleId,
         static_cast<uint32_t>(result));
  sceAppInstUtilTerminate();
  if (result != 0) {
    notify_tile("EZ Cheats - tile FALHOU\nRegistro: 0x%08x",
                static_cast<uint32_t>(result));
    return MediaTileResult::failed;
  }
  notify_tile("EZ Cheats - tile OK\nMidia: EZCH00001\nhttp://127.0.0.1:5911/");
  return assets_current ? MediaTileResult::current
                        : MediaTileResult::installed;
}

}  // namespace ezcheats::platform
