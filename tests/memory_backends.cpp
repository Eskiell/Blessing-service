#include <assert.h>
#include <string.h>

#include "ezcheats/memory/fake_memory_backend.hpp"
#include "ezcheats/memory/memory_backend_factory.hpp"

namespace {

using ezcheats::memory::FakeMemoryBackend;
using ezcheats::memory::MemoryBackendFactory;
using ezcheats::memory::MemoryBackendKind;

void selects_backend_by_firmware_or_override() {
  assert(MemoryBackendFactory::resolve_kind(MemoryBackendKind::automatic,
                                            0x700) ==
         MemoryBackendKind::mdbg);
  assert(MemoryBackendFactory::resolve_kind(MemoryBackendKind::automatic,
                                            0x840) ==
         MemoryBackendKind::kdirect);
  assert(MemoryBackendFactory::resolve_kind(MemoryBackendKind::mdbg, 0x900) ==
         MemoryBackendKind::mdbg);
  assert(MemoryBackendFactory::resolve_kind(MemoryBackendKind::kdirect,
                                            0x500) ==
         MemoryBackendKind::kdirect);
}

void reads_writes_and_verifies() {
  FakeMemoryBackend backend;
  const uint8_t patch[] = {0xde, 0xad, 0xbe, 0xef};
  uint8_t readback[sizeof(patch)]{};
  const uint64_t address = FakeMemoryBackend::kBaseAddress + 32;

  assert(strcmp(backend.name(), "fake") == 0);
  assert(backend.write_verified(42, address, patch, sizeof(patch)));
  assert(backend.read(42, address, readback, sizeof(readback)));
  assert(memcmp(patch, readback, sizeof(patch)) == 0);

  backend.corrupt_after_write(true);
  assert(!backend.write_verified(42, address, patch, sizeof(patch)));
  assert(!backend.read(-1, address, readback, sizeof(readback)));
  assert(!backend.write(42, 0, patch, sizeof(patch)));
  assert(!backend.read(42, FakeMemoryBackend::kBaseAddress +
                               FakeMemoryBackend::kMemorySize - 1,
                       readback, sizeof(readback)));
}

void maps_code_caves_with_valid_inputs() {
  FakeMemoryBackend backend;
  assert(backend.map_code_cave(42, 0x400000, 0x100));
  assert(backend.mapped_cave_count() == 1);
  assert(!backend.map_code_cave(-1, 0x400000, 0x100));
  assert(!backend.map_code_cave(42, 0, 0x100));
  assert(!backend.map_code_cave(42, 0x400000, 0));
  assert(backend.mapped_cave_count() == 1);
}

}  // namespace

int main() {
  selects_backend_by_firmware_or_override();
  reads_writes_and_verifies();
  maps_code_caves_with_valid_inputs();
}
