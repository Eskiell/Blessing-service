#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ezcheats/domain/models.hpp"

namespace ezcheats::domain {

class ICheatParser {
 public:
  virtual bool parse(const uint8_t* data, size_t size, CheatFile& output) = 0;

 protected:
  ~ICheatParser() = default;
};

class IMemoryBackend {
 public:
  virtual bool read(int pid, uint64_t address, void* output, size_t size) = 0;
  virtual bool write(int pid, uint64_t address, const void* input,
                     size_t size) = 0;

 protected:
  ~IMemoryBackend() = default;
};

class IGamePlatform {
 public:
  virtual bool current_game(GameContext& output) = 0;

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
  virtual ServiceState state() const = 0;
  virtual bool set_enabled(uint32_t id, bool enabled,
                           CheatEntry& updated) = 0;

 protected:
  ~ICheatService() = default;
};

}  // namespace ezcheats::domain
