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
#include <sys/stat.h>

#include <ps5/kernel.h>

#ifndef TITLE_ID
#define TITLE_ID "EZST00001"
#endif

#define APP_ROOT "/user/app"
#define APP_PARENT APP_ROOT "/"
#define APP_DIR APP_ROOT "/" TITLE_ID
#define SCE_SYS_DIR APP_DIR "/sce_sys"

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

int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallAll(void *);
int sceAppInstUtilAppUnInstall(const char *);

INCASSET(launcher_param, "param.json");
INCASSET(launcher_icon, "icon0.png");

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
install_launcher(void) {
  int (*install_title_dir)(const char *, const char *, void *) = NULL;
  uint32_t handle = 0;

  if(kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle) == 0) {
    install_title_dir =
        (void *)kernel_dynlib_resolve(-1, handle, "Wudg3Xe3heE");
  }

  if(install_title_dir) {
    return install_title_dir(TITLE_ID, APP_PARENT, NULL);
  }

  return sceAppInstUtilAppInstallAll(NULL);
}

int
main(void) {
  int result = sceAppInstUtilInitialize();
  if(result != 0) {
    printf("sceAppInstUtilInitialize: error 0x%08X\n", result);
    return 1;
  }

  /* Remove a previous registration before refreshing its metadata/assets. */
  sceAppInstUtilAppUnInstall(TITLE_ID);

  if(mkdir_if_needed(APP_DIR) != 0 || mkdir_if_needed(SCE_SYS_DIR) != 0) {
    perror("mkdir launcher");
    return 1;
  }

  if(write_file(SCE_SYS_DIR "/param.json", launcher_param,
                launcher_param_size) != 0) {
    perror("write param.json");
    return 1;
  }

  if(write_file(SCE_SYS_DIR "/icon0.png", launcher_icon,
                launcher_icon_size) != 0) {
    perror("write icon0.png");
    return 1;
  }

  result = install_launcher();
  if(result != 0) {
    printf("install launcher: error 0x%08X\n", result);
    return 1;
  }

  printf("[ezhelit-store] launcher %s installed\n", TITLE_ID);
  printf("[ezhelit-store] deeplink: http://192.168.15.122:5911/\n");
  return 0;
}
