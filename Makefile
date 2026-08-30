PS5_HOST ?= ps5
PS5_PORT ?= 9021
HTTP_PORT ?= 5911
CHEATS_DIRECTORY ?= /data/ez-cheats/cheats
MEMORY_BACKEND ?= automatic
NPM ?= npm
HOST_CXX ?= c++
HOST_CC ?= cc
SHNEXT_KEYSTONE ?= 0

ifneq ($(HTTP_PORT),5911)
$(error HTTP_PORT must be 5911 because the persistent Media tile targets 127.0.0.1:5911)
endif
HOST_TEST_FLAGS := -std=c++20 -Wall -Wextra -Werror -Iinclude -Ithird_party \
	-Ithird_party/shnext -DEZ_CHEATS_HAS_KEYSTONE=0 \
	-fsanitize=address,undefined -fno-omit-frame-pointer
HOST_C_FLAGS := -std=c11 -Wall -Wextra -Werror -Ithird_party \
	-Ithird_party/shnext \
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
MEDIA_PARAM_ASSET := installer/param.json
MEDIA_ICON_ASSET := installer/icon0.png
SOURCES := \
	src/main.cpp \
	src/application/cheat_service.cpp \
	src/application/cheat_applier.cpp \
	src/domain/owned_cheat_file.cpp \
	src/domain/memory_backend.cpp \
	src/parsers/parser_utils.cpp \
	src/parsers/json_cheat_parser.cpp \
	src/parsers/shn_cheat_parser.cpp \
	src/parsers/mc4_cheat_parser.cpp \
	src/parsers/shnext_cheat_parser.cpp \
	src/parsers/shnext_core.cpp \
	src/parsers/cheat_parser_factory.cpp \
	src/repository/file_cheat_repository.cpp \
	src/platform/ps5_game_platform.cpp \
	src/platform/ps5_media_tile.cpp \
	src/memory/ps5_memory_backends.cpp \
	src/http/http_server.cpp \
	src/assets/embedded_frontend.cpp
PARSER_C_OBJECTS := build/ps5/mc4/aes.o build/ps5/mc4/base64.o \
	build/ps5/shnext/miniz.o build/ps5/shnext/sha256.o \
	build/ps5/shnext/cJSON.o
HOST_PARSER_C_OBJECTS := build/host-tests/mc4/aes.o \
	build/host-tests/mc4/base64.o build/host-tests/shnext/miniz.o \
	build/host-tests/shnext/sha256.o build/host-tests/shnext/cJSON.o
HOST_PARSER_SOURCES := src/domain/owned_cheat_file.cpp \
	src/parsers/parser_utils.cpp src/parsers/json_cheat_parser.cpp \
	src/parsers/shn_cheat_parser.cpp src/parsers/mc4_cheat_parser.cpp \
	src/parsers/shnext_cheat_parser.cpp src/parsers/shnext_core.cpp \
	src/parsers/cheat_parser_factory.cpp
HEADERS := $(shell find include -type f -name '*.hpp')

CPPFLAGS := -Iinclude -Ithird_party -Ithird_party/shnext \
	-DEZ_CHEATS_HTTP_PORT=$(HTTP_PORT) \
	-DEZ_CHEATS_DIRECTORY='"$(CHEATS_DIRECTORY)"' \
	-DEZ_CHEATS_FRONTEND_PATH='"$(abspath $(FRONTEND_ASSET))"' \
	-DEZ_CHEATS_MEDIA_PARAM_PATH='"$(abspath $(MEDIA_PARAM_ASSET))"' \
	-DEZ_CHEATS_MEDIA_ICON_PATH='"$(abspath $(MEDIA_ICON_ASSET))"'
CXXFLAGS := -std=c++20 -nostdlib++ -Wall -Wextra -Werror -Os
PS5_LIBS := -lSceSystemService -lSceAppInstUtil -lSceUserService -lSceNet \
	-lSceNetCtl -lpthread

ifeq ($(MEMORY_BACKEND),automatic)
CPPFLAGS += -DEZ_CHEATS_MEMORY_BACKEND=0
else ifeq ($(MEMORY_BACKEND),mdbg)
CPPFLAGS += -DEZ_CHEATS_MEMORY_BACKEND=1
else ifeq ($(MEMORY_BACKEND),kdirect)
CPPFLAGS += -DEZ_CHEATS_MEMORY_BACKEND=2
else
$(error MEMORY_BACKEND must be automatic, mdbg, or kdirect)
endif

ifeq ($(SHNEXT_KEYSTONE),1)
ifeq ($(wildcard $(PS5_PAYLOAD_SDK)/target/lib/libc++.a),)
$(error SHNEXT_KEYSTONE=1 requires a PS5 SDK with libc++.a; use SHNEXT_KEYSTONE=0 for restricted ShnExt support)
endif
CPPFLAGS += -Ithird_party/keystone/include -DEZ_CHEATS_HAS_KEYSTONE=1
SHNEXT_LIBS := third_party/keystone/lib/libkeystone.a
else ifeq ($(SHNEXT_KEYSTONE),0)
CPPFLAGS += -DEZ_CHEATS_HAS_KEYSTONE=0
else
$(error SHNEXT_KEYSTONE must be 0 or 1)
endif

.PHONY: all clean deploy frontend-build host-test

all: $(ELF)

$(FRONTEND_MARKER): $(FRONTEND_DIR)/package.json $(FRONTEND_DIR)/package-lock.json
	cd $(FRONTEND_DIR) && $(NPM) ci

frontend-build: $(FRONTEND_MARKER)
	cd $(FRONTEND_DIR) && $(NPM) run build

$(FRONTEND_ASSET): frontend-build
	@test -s $@

$(ELF): $(SOURCES) $(PARSER_C_OBJECTS) $(HEADERS) $(FRONTEND_ASSET) \
	$(MEDIA_PARAM_ASSET) $(MEDIA_ICON_ASSET)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES) $(PARSER_C_OBJECTS) \
		$(SHNEXT_LIBS) $(PS5_LIBS)

build/ps5/mc4/%.o: third_party/mc4/%.c third_party/mc4/aes.h third_party/mc4/base64.h
	mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -Werror -Ithird_party -c -o $@ $<

build/host-tests/mc4/%.o: third_party/mc4/%.c third_party/mc4/aes.h third_party/mc4/base64.h
	mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_C_FLAGS) -c -o $@ $<

build/ps5/shnext/%.o: third_party/shnext/%.c third_party/shnext/%.h
	mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -Werror -Wno-unused-function \
		-Ithird_party/shnext -c -o $@ $<

build/host-tests/shnext/%.o: third_party/shnext/%.c third_party/shnext/%.h
	mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_C_FLAGS) -Wno-unused-function -c -o $@ $<

build/ps5/shnext/cJSON.o: third_party/shnext/cJSON.cpp third_party/shnext/cJSON.hpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Wno-unreachable-code-generic-assoc \
		-c -o $@ $<

build/host-tests/shnext/cJSON.o: third_party/shnext/cJSON.cpp third_party/shnext/cJSON.hpp
	mkdir -p $(dir $@)
	$(HOST_CXX) $(HOST_TEST_FLAGS) -Wno-deprecated-declarations -c -o $@ $<

host-test: $(HOST_PARSER_C_OBJECTS)
	mkdir -p build/host-tests
	$(HOST_CXX) $(HOST_TEST_FLAGS) -c -o build/host-tests/http_server.o \
		src/http/http_server.cpp
	$(HOST_CXX) $(HOST_TEST_FLAGS) -c -o build/host-tests/main.o src/main.cpp
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/http_routes tests/http_routes.cpp \
		src/application/in_memory_cheat_service.cpp
	./build/host-tests/http_routes
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/json_parser tests/json_parser.cpp \
		$(HOST_PARSER_SOURCES) $(HOST_PARSER_C_OBJECTS)
	./build/host-tests/json_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/shn_parser tests/shn_parser.cpp \
		$(HOST_PARSER_SOURCES) $(HOST_PARSER_C_OBJECTS)
	./build/host-tests/shn_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/mc4_parser tests/mc4_parser.cpp \
		$(HOST_PARSER_SOURCES) $(HOST_PARSER_C_OBJECTS)
	./build/host-tests/mc4_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/shnext_parser tests/shnext_parser.cpp \
		$(HOST_PARSER_SOURCES) $(HOST_PARSER_C_OBJECTS)
	./build/host-tests/shnext_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/file_cheat_repository \
		tests/file_cheat_repository.cpp \
		src/repository/file_cheat_repository.cpp \
		$(HOST_PARSER_SOURCES) $(HOST_PARSER_C_OBJECTS)
	./build/host-tests/file_cheat_repository
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/game_platform tests/game_platform.cpp \
		src/platform/fake_game_platform.cpp
	./build/host-tests/game_platform
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/memory_backends tests/memory_backends.cpp \
		src/domain/memory_backend.cpp src/domain/owned_cheat_file.cpp \
		src/memory/fake_memory_backend.cpp
	./build/host-tests/memory_backends
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/cheat_applier tests/cheat_applier.cpp \
		src/application/cheat_applier.cpp src/domain/memory_backend.cpp \
		src/domain/owned_cheat_file.cpp src/memory/fake_memory_backend.cpp \
		src/platform/fake_game_platform.cpp
	./build/host-tests/cheat_applier
	$(HOST_CXX) $(HOST_TEST_FLAGS) -pthread \
		-o build/host-tests/cheat_service tests/cheat_service.cpp \
		src/application/cheat_service.cpp \
		src/application/cheat_applier.cpp src/domain/memory_backend.cpp \
		src/repository/file_cheat_repository.cpp \
		src/platform/fake_game_platform.cpp \
		src/memory/fake_memory_backend.cpp \
		$(HOST_PARSER_SOURCES) $(HOST_PARSER_C_OBJECTS)
	./build/host-tests/cheat_service

deploy: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $<

clean:
	rm -rf build
	rm -f $(ELF)
	rm -rf $(FRONTEND_DIR)/dist
