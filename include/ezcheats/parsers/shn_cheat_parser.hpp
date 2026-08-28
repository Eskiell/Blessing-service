#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::parsers {

class ShnCheatParser final : public domain::ICheatParser {
 public:
  const char* name() const override { return "shn"; }
  bool parse(const uint8_t* data, size_t size,
             domain::CheatFile& output) override;
};

}  // namespace ezcheats::parsers
