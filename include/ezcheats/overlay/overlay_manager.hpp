#pragma once

namespace ezcheats::overlay {

// Starts one detached, fail-safe injection attempt. A failure must never stop
// the Media tile or HTTP service from starting.
bool start_overlay_injection() noexcept;

}  // namespace ezcheats::overlay
