ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

APP := ezhelit-store-installer.elf
NOTIFY_TEST := ezhelit-notify-test.elf
SRC := install.c
ASSETS := param.json icon0.png
CFLAGS := -Wall -Wextra -Werror -Os -std=c17
LDFLAGS :=
LDLIBS := -lSceNetCtl -lSceUserService -lSceSystemService -lSceAppInstUtil

all: $(APP)

notify-test: $(NOTIFY_TEST)

$(APP): $(SRC) $(ASSETS)
	"$(CC)" $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

$(NOTIFY_TEST): notify-test.c
	"$(CC)" $(CFLAGS) -o $@ $<

clean:
	rm -f $(APP) $(NOTIFY_TEST)

.PHONY: all clean notify-test
