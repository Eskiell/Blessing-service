ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

CC      := $(PS5_PAYLOAD_SDK)/bin/prospero-clang
CFLAGS  := -Wall -Wextra -O2 -std=c17
LDFLAGS :=

APP := ps5-store.elf
SRC := src/main.c

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(APP)

.PHONY: all clean
