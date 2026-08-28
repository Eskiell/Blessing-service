#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::application {

class InMemoryCheatService final : public domain::ICheatService {
 public:
  InMemoryCheatService() noexcept;

  domain::ServiceState state() const override;
  bool set_enabled(uint32_t id, bool enabled,
                   domain::CheatEntry& updated) override;

 private:
  domain::GameContext game_{};
  domain::CheatEntry cheats_[3]{};
};

}  // namespace ezcheats::application
