#include "ezcheats/domain/interfaces.hpp"

#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"

namespace ezcheats::domain {

bool IMemoryBackend::write_verified(int pid, uint64_t address,
                                    const void* input, size_t size) {
  if (pid < 0 || address == 0 || input == nullptr || size == 0) {
    return false;
  }
  void* verification = malloc(size);
  if (verification == nullptr) return false;
  const bool matches = write(pid, address, input, size) &&
                       read(pid, address, verification, size) &&
                       memcmp(input, verification, size) == 0;
  secure_zero(verification, size);
  free(verification);
  return matches;
}

}  // namespace ezcheats::domain
