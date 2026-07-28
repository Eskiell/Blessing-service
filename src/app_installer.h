#pragma once

/*
 * Ensures the EZHELIT Store Media tile exists and matches the embedded assets.
 *
 * Returns:
 *   1 when the tile was installed or refreshed;
 *   0 when it was already up to date;
 *  -1 on failure.
 */
int app_install_if_needed(void);
