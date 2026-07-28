#include "app_installer.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <ps5/kernel.h>

#define TITLE_ID "EZST00001"
#define APP_DIR "/user/app/" TITLE_ID
#define SCE_SYS_DIR APP_DIR "/sce_sys"
#define PARAM_PATH SCE_SYS_DIR "/param.json"
#define ICON_PATH SCE_SYS_DIR "/icon0.png"

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

INCASSET(param_json, "installer/param.json");
INCASSET(icon0_png, "installer/icon0.png");

typedef struct notify_request {
  char reserved[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);
int sceNetCtlInit(void);
int sceUserServiceInitialize(void *);
int sceAppInstUtilInitialize(void);
int sceAppInstUtilTerminate(void);
int sceAppInstUtilAppInstallAll(void *);

static void
notify(const char *message) {
  notify_request_t request;
  memset(&request, 0, sizeof(request));
  snprintf(request.message, sizeof(request.message), "%s", message);
  sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

static int
file_matches(const char *path, const uint8_t *expected, size_t expected_size) {
  struct stat info;
  if(stat(path, &info) != 0 || (size_t)info.st_size != expected_size) return 0;

  FILE *file = fopen(path, "rb");
  if(!file) return 0;

  uint8_t *data = malloc(expected_size);
  if(!data) {
    fclose(file);
    return 0;
  }

  size_t read_size = fread(data, 1, expected_size, file);
  int close_result = fclose(file);
  int matches = read_size == expected_size && close_result == 0 &&
                memcmp(data, expected, expected_size) == 0;
  free(data);
  return matches;
}

static int
write_file(const char *path, const uint8_t *data, size_t size) {
  FILE *file = fopen(path, "wb");
  if(!file) return -1;

  size_t written = fwrite(data, 1, size, file);
  int close_result = fclose(file);
  return written == size && close_result == 0 ? 0 : -1;
}

static int
install_title(void) {
  int (*install_title_dir)(const char *, const char *, void *) = NULL;
  uint32_t handle = 0;

  if(kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle) == 0) {
    install_title_dir =
        (void *)kernel_dynlib_resolve(-1, handle, "Wudg3Xe3heE");
  }

  if(install_title_dir) {
    return install_title_dir(TITLE_ID, "/user/app/", NULL);
  }
  return sceAppInstUtilAppInstallAll(NULL);
}

int
app_install_if_needed(void) {
  int param_current =
      file_matches(PARAM_PATH, param_json, param_json_size);
  int icon_current =
      file_matches(ICON_PATH, icon0_png, icon0_png_size);
  if(param_current && icon_current) return 0;

  notify("EZHELIT Store: preparando tile em Media");

  sceNetCtlInit();
  int user_priority = 256;
  sceUserServiceInitialize(&user_priority);

  int result = sceAppInstUtilInitialize();
  if(result != 0) {
    notify("EZHELIT Store: AppInstUtil falhou");
    return -1;
  }

  if((mkdir(APP_DIR, 0755) != 0 && errno != EEXIST) ||
     (mkdir(SCE_SYS_DIR, 0755) != 0 && errno != EEXIST)) {
    notify("EZHELIT Store: falha ao criar diretorios");
    sceAppInstUtilTerminate();
    return -1;
  }

  if(write_file(PARAM_PATH, param_json, param_json_size) != 0 ||
     write_file(ICON_PATH, icon0_png, icon0_png_size) != 0) {
    notify("EZHELIT Store: falha ao gravar assets");
    sceAppInstUtilTerminate();
    return -1;
  }

  result = install_title();
  sceAppInstUtilTerminate();
  if(result != 0) {
    char message[160];
    snprintf(message, sizeof(message),
             "EZHELIT Store: registro falhou 0x%08X", result);
    notify(message);
    return -1;
  }

  notify("EZHELIT Store: tile pronto em Media");
  return 1;
}
