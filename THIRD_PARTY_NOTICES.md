# Third-party notices

## OnionHEN cheat engine

The cheat parsing code under `src/parsers`, its supporting domain allocation
code, and the JSON fixture were adapted from OnionHEN:

- Project: [Eskiell/onionHEN](https://github.com/Eskiell/onionHEN)
- Source revision: `5110659f4f1b5c8e4f8b61a78505ff7923b73778`
- Source paths:
  - `source/util/source/cheats/cheat_engine_utils.c`
  - `source/util/source/cheats/json_cheat_parser.cpp`
  - `source/util/source/cheats/cheat_parser_factory.cpp`
  - `source/util/tests/test_cheat_parsers.cpp`
  - `source/util/tests/fixtures/cheats/PPSA26344_01.008.000.json`
- License: GNU General Public License, version 3

The EZ Cheats adaptation replaces OnionHEN-specific logging, paths and utility
dependencies, uses the EZ Cheats domain model, adds explicit ownership and
bounded parsing, and rejects unregistered formats.
