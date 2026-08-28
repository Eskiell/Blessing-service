#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::parsers {

class ShnExtCheatParser final : public domain::ICheatParser {
 public:
  const char* name() const override { return "ShnExt"; }
  bool parse(const uint8_t* data, size_t size,
             domain::CheatFile& output) override;
};

}  // namespace ezcheats::parsers
