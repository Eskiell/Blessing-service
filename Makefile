ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

PAYLOAD := ezhelit-store.elf
PAYLOAD_SRC := src/main.c src/app_installer.c
PAYLOAD_CFLAGS := -Wall -Wextra -Werror -Os -std=c17
PAYLOAD_LIBS := -lpthread -lSceNetCtl -lSceUserService -lSceSystemService -lSceAppInstUtil
LAUNCHER_ASSETS := installer/param.json installer/icon0.png

all: payload

payload: $(PAYLOAD)

$(PAYLOAD): $(PAYLOAD_SRC) $(LAUNCHER_ASSETS)
	"$(CC)" $(PAYLOAD_CFLAGS) -o $@ $(PAYLOAD_SRC) $(PAYLOAD_LIBS)

clean:
	rm -f $(PAYLOAD)
	rm -f installer/ezhelit-store-installer.elf

.PHONY: all payload clean
