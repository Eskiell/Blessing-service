#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::parsers {

class JsonCheatParser final : public domain::ICheatParser {
 public:
  const char* name() const override { return "json"; }
  bool parse(const uint8_t* data, size_t size,
             domain::CheatFile& output) override;
};

}  // namespace ezcheats::parsers
