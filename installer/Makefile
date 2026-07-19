ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

CC      := $(PS5_PAYLOAD_SDK)/bin/prospero-clang
CFLAGS  := -Wall -Wextra -O2 -std=c17
LDFLAGS := -lSceAppInstUtil

APP := ps5-store-install.elf
SRC := install.c

all: $(APP)

# depends on ../ps5-store.elf - build that first (top-level make)
$(APP): $(SRC) ../ps5-store.elf assets/param.json assets/icon0.png
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f $(APP)

.PHONY: all clean
