PS5_HOST ?= ps5
PS5_PORT ?= 9021
HTTP_PORT ?= 5911
NPM ?= npm
HOST_CXX ?= c++
HOST_CC ?= cc
HOST_TEST_FLAGS := -std=c++20 -Wall -Wextra -Werror -Iinclude -Ithird_party \
	-fsanitize=address,undefined -fno-omit-frame-pointer
HOST_C_FLAGS := -std=c11 -Wall -Wextra -Werror -Ithird_party \
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
	src/parsers/mc4_cheat_parser.cpp \
	src/parsers/cheat_parser_factory.cpp \
	src/http/http_server.cpp \
	src/assets/embedded_frontend.cpp
C_SOURCES := third_party/mc4/aes.c third_party/mc4/base64.c
MC4_OBJECTS := build/ps5/mc4/aes.o build/ps5/mc4/base64.o
HOST_MC4_OBJECTS := build/host-tests/mc4/aes.o build/host-tests/mc4/base64.o
HEADERS := $(shell find include -type f -name '*.hpp')

CPPFLAGS := -Iinclude -Ithird_party -DEZ_CHEATS_HTTP_PORT=$(HTTP_PORT) \
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

$(ELF): $(SOURCES) $(MC4_OBJECTS) $(HEADERS) $(FRONTEND_ASSET)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES) $(MC4_OBJECTS)

build/ps5/mc4/%.o: third_party/mc4/%.c third_party/mc4/aes.h third_party/mc4/base64.h
	mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -Werror -Ithird_party -c -o $@ $<

build/host-tests/mc4/%.o: third_party/mc4/%.c third_party/mc4/aes.h third_party/mc4/base64.h
	mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_C_FLAGS) -c -o $@ $<

host-test: $(HOST_MC4_OBJECTS)
	mkdir -p build/host-tests
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/http_routes tests/http_routes.cpp \
		src/application/in_memory_cheat_service.cpp
	./build/host-tests/http_routes
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/json_parser tests/json_parser.cpp \
		src/domain/owned_cheat_file.cpp src/parsers/parser_utils.cpp \
		src/parsers/json_cheat_parser.cpp src/parsers/shn_cheat_parser.cpp \
		src/parsers/mc4_cheat_parser.cpp \
		src/parsers/cheat_parser_factory.cpp $(HOST_MC4_OBJECTS)
	./build/host-tests/json_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/shn_parser tests/shn_parser.cpp \
		src/domain/owned_cheat_file.cpp src/parsers/parser_utils.cpp \
		src/parsers/json_cheat_parser.cpp src/parsers/shn_cheat_parser.cpp \
		src/parsers/mc4_cheat_parser.cpp \
		src/parsers/cheat_parser_factory.cpp $(HOST_MC4_OBJECTS)
	./build/host-tests/shn_parser
	$(HOST_CXX) $(HOST_TEST_FLAGS) \
		-o build/host-tests/mc4_parser tests/mc4_parser.cpp \
		src/domain/owned_cheat_file.cpp src/parsers/parser_utils.cpp \
		src/parsers/json_cheat_parser.cpp src/parsers/shn_cheat_parser.cpp \
		src/parsers/mc4_cheat_parser.cpp src/parsers/cheat_parser_factory.cpp \
		$(HOST_MC4_OBJECTS)
	./build/host-tests/mc4_parser

deploy: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $<

clean:
	rm -rf build
	rm -f $(ELF)
	rm -rf $(FRONTEND_DIR)/dist
