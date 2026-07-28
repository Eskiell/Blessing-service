ifeq ($(PS5_PAYLOAD_SDK),)
$(error PS5_PAYLOAD_SDK is undefined. export PS5_PAYLOAD_SDK=/path/to/sdk)
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

PAYLOAD := ezhelit-store.elf
PAYLOAD_SRC := src/main.c src/app_installer.c
PAYLOAD_CFLAGS := -Wall -Wextra -Werror -Os -std=c17
PAYLOAD_LIBS := -lpthread -lSceNetCtl -lSceUserService -lSceSystemService -lSceAppInstUtil
LAUNCHER_ASSETS := installer/param.json installer/icon0.png
NPM ?= npm
UI_ASSET ?= frontend/dist/index.html
LEGACY_UI_ASSET := frontend/legacy/index.html
LEGACY_PAYLOAD := ezhelit-store-legacy-ui.elf

all: frontend-build payload

payload: $(PAYLOAD)

frontend/node_modules/.package-lock.json: frontend/package.json
	cd frontend && $(NPM) install

.PHONY: frontend-build
frontend-build: frontend/node_modules/.package-lock.json
	cd frontend && $(NPM) run build

frontend/dist/index.html: frontend-build
	@test -f $@

$(PAYLOAD): $(PAYLOAD_SRC) $(LAUNCHER_ASSETS) $(UI_ASSET)
	"$(CC)" $(PAYLOAD_CFLAGS) -DUI_ASSET_PATH=\"$(UI_ASSET)\" -o $@ $(PAYLOAD_SRC) $(PAYLOAD_LIBS)

legacy-ui: $(LEGACY_PAYLOAD)

$(LEGACY_PAYLOAD): $(PAYLOAD_SRC) $(LAUNCHER_ASSETS) $(LEGACY_UI_ASSET)
	"$(CC)" $(PAYLOAD_CFLAGS) -DUI_ASSET_PATH=\"$(LEGACY_UI_ASSET)\" -o $@ $(PAYLOAD_SRC) $(PAYLOAD_LIBS)

clean:
	rm -f $(PAYLOAD) $(LEGACY_PAYLOAD)
	rm -f installer/ezhelit-store-installer.elf

.PHONY: all payload legacy-ui clean
