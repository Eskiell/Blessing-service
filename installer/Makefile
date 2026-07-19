ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

APP := ezhelit-store-installer.elf
SRC := install.c
ASSETS := param.json icon0.png
CFLAGS := -Wall -Wextra -Werror -Os -std=c17
LDFLAGS :=
LDLIBS := -lSceNetCtl -lSceUserService -lSceSystemService -lSceAppInstUtil

all: $(APP)

$(APP): $(SRC) $(ASSETS)
	"$(CC)" $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f $(APP)

.PHONY: all clean
