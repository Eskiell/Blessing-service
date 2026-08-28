#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::memory {

class FakeMemoryBackend final : public domain::IMemoryBackend {
 public:
  static constexpr uint64_t kBaseAddress = 0x100000;
  static constexpr size_t kMemorySize = 4096;

  const char* name() const override { return "fake"; }
  bool read(int pid, uint64_t address, void* output, size_t size) override;
  bool write(int pid, uint64_t address, const void* input,
             size_t size) override;
  bool map_code_cave(int pid, uint64_t address, size_t size) override;

  void corrupt_after_write(bool enabled) noexcept {
    corrupt_after_write_ = enabled;
  }
  size_t mapped_cave_count() const noexcept { return mapped_cave_count_; }

 private:
  bool range(uint64_t address, size_t size, size_t& offset) const noexcept;

  uint8_t memory_[kMemorySize]{};
  size_t mapped_cave_count_ = 0;
  bool corrupt_after_write_ = false;
};

}  // namespace ezcheats::memory
