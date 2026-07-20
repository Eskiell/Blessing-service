ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

PAYLOAD := ezhelit-store.elf
PAYLOAD_SRC := src/main.c
PAYLOAD_CFLAGS := -Wall -Wextra -Werror -Os -std=c17
PAYLOAD_LIBS := -lpthread

all: payload installer

payload: $(PAYLOAD)

$(PAYLOAD): $(PAYLOAD_SRC)
	"$(CC)" $(PAYLOAD_CFLAGS) -o $@ $< $(PAYLOAD_LIBS)

installer:
	$(MAKE) -C installer

clean:
	rm -f $(PAYLOAD)
	$(MAKE) -C installer clean

.PHONY: all payload installer clean
