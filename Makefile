PS5_HOST ?= ps5
PS5_PORT ?= 9021
HTTP_PORT ?= 5911
NPM ?= npm
HOST_CXX ?= c++
HOST_TEST_FLAGS := -std=c++20 -Wall -Wextra -Werror -Iinclude \
	-fsanitize=address,undefined -fno-omit-frame-pointer

ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
$(error PS5_PAYLOAD_SDK is undefined. Export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk)
endif

ELF := ez-cheats.elf
FRONTEND_DIR := frontend
FRONTEND_ASSET := $(FRONTEND_DIR)/dist/index.html
FRONTEND_MARKER := $(FRONTEND_DIR)/node_modules/.package-lock.json
SOURCES := \
	src/main.cpp \
	src/application/in_memory_cheat_service.cpp \
	src/domain/owned_cheat_file.cpp \
	src/parsers/parser_utils.cpp \
	src/parsers/json_cheat_parser.cpp \
	src/parsers/shn_cheat_parser.cpp \
	src/parsers/cheat_parser_factory.cpp \
	src/http/http_server.cpp \
	src/assets/embedded_frontend.cpp
HEADERS := $(shell find include -type f -name '*.hpp')

CPPFLAGS := -Iinclude -DEZ_CHEATS_HTTP_PORT=$(HTTP_PORT) \
	-DEZ_CHEATS_FRONTEND_PATH='"$(abspath $(FRONTEND_ASSET))"'
CXXFLAGS := -std=c++20 -nostdlib++ -Wall -Wextra -Werror -Os

.PHONY: all clean deploy frontend-build host-test

all: $(ELF)

$(FRONTEND_MARKER): $(FRONTEND_DIR)/package.json $(FRONTEND_DIR)/package-lock.json
	cd $(FRONTEND_DIR) && $(NPM) ci

frontend-build: $(FRONTEND_MARKER)
	cd $(FRONTEND_DIR) && $(NPM) run build

$(FRONTEND_ASSET): frontend-build
	@test -s $@

$(ELF): $(SOURCES) $(HEADERS) $(FRONTEND_ASSET)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES)

host-test:
	mkdir -p build/host-tests
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/http_routes tests/http_routes.cpp \
		src/application/in_memory_cheat_service.cpp
	./build/host-tests/http_routes
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/json_parser tests/json_parser.cpp \
		src/domain/owned_cheat_file.cpp src/parsers/parser_utils.cpp \
		src/parsers/json_cheat_parser.cpp src/parsers/shn_cheat_parser.cpp \
		src/parsers/cheat_parser_factory.cpp
	./build/host-tests/json_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/shn_parser tests/shn_parser.cpp \
		src/domain/owned_cheat_file.cpp src/parsers/parser_utils.cpp \
		src/parsers/json_cheat_parser.cpp src/parsers/shn_cheat_parser.cpp \
		src/parsers/cheat_parser_factory.cpp
	./build/host-tests/shn_parser

deploy: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $<

clean:
	rm -rf build
	rm -f $(ELF)
	rm -rf $(FRONTEND_DIR)/dist
