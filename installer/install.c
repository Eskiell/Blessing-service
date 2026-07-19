/*
 * ps5-store installer
 *
 * Adapted from ps5-payload-dev/ftpsrv's install-ps5.c (GPLv3), by
 * John Törnblom. Original: https://github.com/ps5-payload-dev/ftpsrv/blob/master/install-ps5.c
 *
 * What this does: embeds ps5-store.elf + icon0.png + param.json into
 * this binary at build time, then writes them out to /user/app/<TITLE_ID>/
 * and calls into the system's app-install routine (libSceAppInstUtil,
 * resolved directly from the kernel's loaded module table) so the PS5
 * registers it as a real home-screen tile.
 *
 * Run this ONCE (or whenever ps5-store.elf changes) via your ELF
 * loader. It doesn't start the store itself - tapping the resulting
 * tile on the PS5 home screen does that.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ps5/kernel.h>

#ifndef TITLE_ID
#define TITLE_ID "HBST00001"
#endif

#define INCASSET(name, file) \
  __asm__(".section .rodata\n" \
          ".global " #name "\n" \
          ".global " #name "_end\n" \
          ".global " #name "_size\n" \
          ".align 16\n" \
          #name ":\n" \
          ".incbin \"" file "\"\n" \
          #name "_end:\n" \
          #name "_size:\n" \
          ".quad " #name "_end - " #name "\n" \
          ".previous\n"); \
  extern const uint8_t name[]; \
  extern const size_t name##_size;

int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallAll(void*);
int sceAppInstUtilAppUnInstall(const char*);

INCASSET(payload, "../ps5-store.elf");
INCASSET(param, "assets/param.json");
INCASSET(icon0, "assets/icon0.png");

static int
install_file(const char* path, const uint8_t* data, size_t size) {
  FILE* f;

  if (!(f = fopen(path, "w"))) {
    return -1;
  }

  if (fwrite(data, size, 1, f) != 1) {
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}

static int
install_app(const char* title_id, const char* dir) {
  int (*sceAppInstUtilAppInstallTitleDir)(const char*, const char*, void*) = 0;
  const char* nid = "Wudg3Xe3heE";
  uint32_t handle;

  if (!kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle)) {
    sceAppInstUtilAppInstallTitleDir = (void*)kernel_dynlib_resolve(-1, handle, nid);
  }

  if (sceAppInstUtilAppInstallTitleDir) {
    return sceAppInstUtilAppInstallTitleDir(title_id, dir, 0);
  }

  return sceAppInstUtilAppInstallAll(0);
}

int
main(int argc, char *argv[]) {
  int err;

  if ((err = sceAppInstUtilInitialize())) {
    printf("sceAppInstUtilInitialize: error 0x%08X\n", err);
    return -1;
  }

  /* wipe any previous install of this title so updates don't collide */
  sceAppInstUtilAppUnInstall(TITLE_ID);

  if (mkdir("/user/app/" TITLE_ID, 0755)) {
    perror("mkdir");
    return -1;
  }

  if (mkdir("/user/app/" TITLE_ID "/sce_sys", 0755)) {
    perror("mkdir");
    return -1;
  }

  if (install_file("/user/app/" TITLE_ID "/eboot.elf", payload, payload_size)) {
    perror("install_file eboot.elf");
    return -1;
  }

  if (install_file("/user/app/" TITLE_ID "/sce_sys/icon0.png", icon0, icon0_size)) {
    perror("install_file icon0.png");
    return -1;
  }

  if (install_file("/user/app/" TITLE_ID "/sce_sys/param.json", param, param_size)) {
    perror("install_file param.json");
    return -1;
  }

  if ((err = install_app(TITLE_ID, "/user/app/"))) {
    printf("install_app: error 0x%08X\n", err);
    return -1;
  }

  printf("[ps5-store-installer] tile installed as %s\n", TITLE_ID);
  return 0;
}
