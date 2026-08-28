#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ezcheats/domain/models.hpp"

namespace ezcheats::domain {

class ICheatParser {
 public:
  virtual const char* name() const = 0;
  virtual bool parse(const uint8_t* data, size_t size, CheatFile& output) = 0;

 protected:
  ~ICheatParser() = default;
};

class IMemoryBackend {
 public:
  virtual const char* name() const = 0;
  virtual bool read(int pid, uint64_t address, void* output, size_t size) = 0;
  virtual bool write(int pid, uint64_t address, const void* input,
                     size_t size) = 0;
  virtual bool map_code_cave(int pid, uint64_t address, size_t size) = 0;
  bool write_verified(int pid, uint64_t address, const void* input,
                      size_t size);

 protected:
  ~IMemoryBackend() = default;
};

class IGamePlatform {
 public:
  virtual bool current_game(GameContext& output) = 0;
  virtual bool find_module(int pid, const char* module_name,
                           ModuleInfo& output) = 0;
  virtual bool find_module_in_app(int app_id, const char* module_name,
                                  int& pid, ModuleInfo& output) = 0;

 protected:
  ~IGamePlatform() = default;
};

class ICheatRepository {
 public:
  virtual bool load(const GameContext& game, CheatFile& output) = 0;

 protected:
  ~ICheatRepository() = default;
};

class ICheatService {
 public:
  virtual bool refresh() = 0;
  virtual bool snapshot(ServiceSnapshot& output) const = 0;
  virtual bool set_enabled(uint32_t id, bool enabled,
                           CheatEntry& updated) = 0;

 protected:
  ~ICheatService() = default;
};

}  // namespace ezcheats::domain
