#pragma once

namespace ezcheats::platform {

enum class MediaTileResult {
  installed,
  current,
  failed,
};

MediaTileResult install_media_tile_if_needed() noexcept;

}  // namespace ezcheats::platform
