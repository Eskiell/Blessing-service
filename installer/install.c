/*
 * EZHELIT Store launcher installer.
 *
 * Installs a PS5 Media launcher made only of param.json and icon0.png.
 * The installed launcher contains no eboot.elf and only opens the deeplink
 * declared in param.json.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <ps5/kernel.h>

#ifndef TITLE_ID
#define TITLE_ID "EZST00001"
#endif

#define APP_ROOT "/user/app"
#define APP_PARENT APP_ROOT "/"
#define APP_DIR APP_ROOT "/" TITLE_ID
#define SCE_SYS_DIR APP_DIR "/sce_sys"
#define APPINST_MODULE "libSceAppInstUtil.sprx"
#define APPINST_PATH "/system/common/lib/libSceAppInstUtil.sprx"
#define NID_INSTALL_TITLE_DIR "Wudg3Xe3heE"

#define INCASSET(name, file)                                                  \
  __asm__(".section .rodata\n"                                                \
          ".global " #name "\n"                                               \
          ".global " #name "_end\n"                                           \
          ".global " #name "_size\n"                                          \
          ".align 16\n"                                                        \
          #name ":\n"                                                         \
          ".incbin \"" file "\"\n"                                            \
          #name "_end:\n"                                                     \
          #name "_size:\n"                                                    \
          ".quad " #name "_end - " #name "\n"                                \
          ".previous\n");                                                     \
  extern const uint8_t name[];                                                \
  extern const size_t name##_size

typedef struct notify_request {
  char reserved[45];
  char message[3075];
} notify_request_t;

typedef int (*appinst_initialize_fn)(void);
typedef int (*appinst_install_title_dir_fn)(const char *, const char *, void *);
typedef int (*appinst_install_all_fn)(void *);
typedef int (*appinst_uninstall_fn)(const char *);

typedef struct appinst_api {
  appinst_initialize_fn initialize;
  appinst_install_title_dir_fn install_title_dir;
  appinst_install_all_fn install_all;
  appinst_uninstall_fn uninstall;
} appinst_api_t;

int sceKernelLoadStartModule(const char *, size_t, const void *, uint32_t,
                             void *, int *);
int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);

INCASSET(launcher_param, "param.json");
INCASSET(launcher_icon, "icon0.png");

static void
notify(const char *message) {
  notify_request_t request;
  memset(&request, 0, sizeof(request));
  snprintf(request.message, sizeof(request.message), "%s", message);
  sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

static void *
resolve_symbol(uint32_t handle, const char *symbol) {
  return (void *)kernel_dynlib_dlsym(-1, handle, symbol);
}

static int
resolve_appinst(appinst_api_t *api) {
  uint32_t handle = 0;
  memset(api, 0, sizeof(*api));

  int result = kernel_dynlib_handle(-1, APPINST_MODULE, &handle);
  if(result != 0) {
    int module =
        sceKernelLoadStartModule(APPINST_PATH, 0, NULL, 0, NULL, NULL);
    if(module > 0) {
      handle = (uint32_t)module;
    } else if(kernel_dynlib_handle(-1, APPINST_MODULE, &handle) != 0) {
      return -1;
    }
  }

  api->initialize =
      (appinst_initialize_fn)resolve_symbol(handle,
                                            "sceAppInstUtilInitialize");
  api->install_title_dir = (appinst_install_title_dir_fn)
      kernel_dynlib_resolve(-1, handle, NID_INSTALL_TITLE_DIR);
  api->install_all =
      (appinst_install_all_fn)resolve_symbol(handle,
                                             "sceAppInstUtilAppInstallAll");
  api->uninstall =
      (appinst_uninstall_fn)resolve_symbol(handle,
                                           "sceAppInstUtilAppUnInstall");

  return api->initialize && (api->install_title_dir || api->install_all)
             ? 0
             : -1;
}

static int
mkdir_if_needed(const char *path) {
  if(mkdir(path, 0755) == 0) {
    return 0;
  }
  return errno == EEXIST ? 0 : -1;
}

static int
write_file(const char *path, const uint8_t *data, size_t size) {
  FILE *file = fopen(path, "wb");
  if(!file) {
    return -1;
  }

  size_t written = fwrite(data, 1, size, file);
  int close_result = fclose(file);
  return written == size && close_result == 0 ? 0 : -1;
}

static int
install_launcher(const appinst_api_t *api) {
  if(api->install_title_dir) {
    int result = api->install_title_dir(TITLE_ID, APP_PARENT, NULL);
    if(result == 0) {
      return 0;
    }
  }

  return api->install_all ? api->install_all(NULL) : -1;
}

int
main(void) {
  appinst_api_t api;
  notify("EZHELIT Store: iniciando instalacao");

  if(resolve_appinst(&api) != 0) {
    notify("EZHELIT Store: falha ao carregar AppInstUtil");
    return 1;
  }

  int result = api.initialize();
  if(result != 0) {
    printf("sceAppInstUtilInitialize: error 0x%08X\n", result);
    notify("EZHELIT Store: falha ao inicializar instalador");
    return 1;
  }

  /* Remove a previous registration before refreshing its metadata/assets. */
  if(api.uninstall) {
    api.uninstall(TITLE_ID);
  }

  if(mkdir_if_needed(APP_DIR) != 0 || mkdir_if_needed(SCE_SYS_DIR) != 0) {
    perror("mkdir launcher");
    notify("EZHELIT Store: falha ao criar diretorio");
    return 1;
  }

  if(write_file(SCE_SYS_DIR "/param.json", launcher_param,
                launcher_param_size) != 0) {
    perror("write param.json");
    notify("EZHELIT Store: falha ao gravar param.json");
    return 1;
  }

  if(write_file(SCE_SYS_DIR "/icon0.png", launcher_icon,
                launcher_icon_size) != 0) {
    perror("write icon0.png");
    notify("EZHELIT Store: falha ao gravar icon0.png");
    return 1;
  }

  result = install_launcher(&api);
  if(result != 0) {
    printf("install launcher: error 0x%08X\n", result);
    notify("EZHELIT Store: falha ao registrar tile");
    return 1;
  }

  printf("[ezhelit-store] launcher %s installed\n", TITLE_ID);
  printf("[ezhelit-store] deeplink: http://192.168.15.122:5911/\n");
  notify("EZHELIT Store instalado. Verifique Jogos e Media.");
  return 0;
}
