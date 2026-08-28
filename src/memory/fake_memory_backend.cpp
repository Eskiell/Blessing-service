#include "ezcheats/memory/fake_memory_backend.hpp"

#include <string.h>

namespace ezcheats::memory {

bool FakeMemoryBackend::range(uint64_t address, size_t size,
                              size_t& offset) const noexcept {
  if (address < kBaseAddress || size == 0) return false;
  const uint64_t distance = address - kBaseAddress;
  if (distance >= kMemorySize || size > kMemorySize - distance) return false;
  offset = static_cast<size_t>(distance);
  return true;
}

bool FakeMemoryBackend::read(int pid, uint64_t address, void* output,
                             size_t size) {
  size_t offset = 0;
  if (pid < 0 || output == nullptr || !range(address, size, offset)) {
    return false;
  }
  memcpy(output, memory_ + offset, size);
  return true;
}

bool FakeMemoryBackend::write(int pid, uint64_t address, const void* input,
                              size_t size) {
  size_t offset = 0;
  if (pid < 0 || input == nullptr || !range(address, size, offset)) {
    return false;
  }
  memcpy(memory_ + offset, input, size);
  if (corrupt_after_write_) memory_[offset] ^= 0xff;
  return true;
}

bool FakeMemoryBackend::map_code_cave(int pid, uint64_t address, size_t size) {
  if (pid < 0 || address == 0 || size == 0) return false;
  ++mapped_cave_count_;
  return true;
}

}  // namespace ezcheats::memory
