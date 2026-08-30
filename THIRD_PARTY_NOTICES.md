# Third-party notices

## PS5 Media tile installer

The Media-category tile installation flow in
`src/platform/ps5_media_tile.cpp` was adapted from EZHELIT-TORRENT:

- Local reference: `/Users/ezequiel/Downloads/EZHELIT-TORRENT`
- Source paths: `engine/app_installer.cpp`, `engine/app_installer.hpp`,
  `engine/app_config.hpp`, and `installer/param.json`
- License: GNU General Public License, version 3

The Blessing adaptation uses its own title ID, manifest, original angel icon,
namespace, paths and non-fatal startup integration. No torrent functionality or
runtime dependency was imported.

## OnionHEN cheat engine

The cheat parsing code under `src/parsers`, its supporting domain allocation
code, and the JSON fixture were adapted from OnionHEN:

- Project: [Eskiell/onionHEN](https://github.com/Eskiell/onionHEN)
- Source revision: `5110659f4f1b5c8e4f8b61a78505ff7923b73778`
- Source paths:
  - `source/util/source/cheats/cheat_engine_utils.c`
  - `source/util/source/cheats/json_cheat_parser.cpp`
  - `source/util/source/cheats/xml_cheat_parser.cpp`
  - `source/util/source/cheats/mc4_cheat_parser.cpp`
  - `source/util/source/cheats/cheat_engine_parser_shnext.c`
  - `source/util/source/cheats/shn_ext_cheat_parser.cpp`
  - `source/util/source/cheats/cheat_repository.cpp`
  - `source/util/source/util_platform.c`
  - `source/util/include/util_platform.h`
  - `source/util/source/cheats/memory_backends.cpp`
  - `source/util/include/cheats/i_memory_backend.hpp`
  - `source/util/source/cheats/cheat_applier.cpp`
  - `source/util/source/cheats/cheat_service.cpp`
  - `source/util/source/util_platform.c` (`find_eboot_imagebase` fallback)
  - `third_party/cheat_support/aes.c`
  - `third_party/cheat_support/base64.c`
  - `source/util/source/cheats/cheat_parser_factory.cpp`
  - `source/util/tests/test_cheat_parsers.cpp`
  - `source/util/tests/fixtures/cheats/PPSA26344_01.008.000.json`
  - `source/util/tests/fixtures/cheats/PPSA21159_01.001.000.shn`
  - `source/util/tests/fixtures/cheats/PPSA08710_01.005.000.mc4`
  - `source/util/tests/fixtures/cheats/Assassins-Creed-Mirage_PPSA07230_01.012.000_Aigars_Uze.ShnExt`
- License: GNU General Public License, version 3

The Blessing adaptation replaces OnionHEN-specific logging, paths and utility
dependencies, uses the Blessing domain model, adds explicit ownership and
bounded parsing, and rejects unregistered formats.

The Base64 implementation carries its original copyright notice and BSD
permission statement in `third_party/mc4/base64.c`. The AES implementation is
vendored from the OnionHEN revision identified above and remains distributed as
part of this GPLv3 project.

## ShnExt support libraries

The ShnExt parser vendors the cJSON, miniz and SHA-256 sources used by the
OnionHEN revision above under `third_party/shnext`. cJSON retains its MIT
license header, miniz is distributed under its embedded public-domain/
Unlicense terms, and the SHA-256 implementation retains its original Brad
Conte attribution.

Keystone Engine is vendored as headers and a PS5 x86-64 static archive under
`third_party/keystone`. It can be enabled in the PS5 payload to assemble
x86-64 patch expressions. The archive adds approximately 4.1 MB to the source
tree and requires a PS5 SDK that supplies `libc++.a`. Host tests and the default
payload build use `SHNEXT_KEYSTONE=0`; in that mode ShnExt remains available
for `nop:<count>` patches while other assembly expressions are rejected.
Use `SHNEXT_KEYSTONE=1` with a compatible SDK to enable generic Assembly. An
incompatible SDK fails immediately with an actionable build error.

- Project: [Keystone Engine](https://www.keystone-engine.org/)
- License: GNU General Public License, version 2
