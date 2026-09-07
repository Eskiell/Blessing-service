# In-game overlay plan

The Blessing overlay is an optional ShellUI module. It does not own cheat
parsing, process discovery, module resolution, or memory writes. Those remain
in the existing `blessing.elf` and `CheatService`.

## Architecture

```text
Controller
   |
   v
blessing_overlay.elf (SceShellUI)
   |  IOverlayClient over loopback HTTP, replaceable by native IPC
   v
blessing.elf -> CheatService -> mdbg / kdirect -> game memory
```

The overlay binary will be embedded in `blessing.elf`, so deployment continues
to require only one user-facing payload. The Media tile and Vue frontend remain
available as a fallback.

## Safety boundaries

- ShellUI code renders UI and translates controller input only.
- The overlay never writes game memory directly.
- The global shortcut is observed without consuming controller input.
- While the overlay is closed, only the shortcut is polled at a bounded rate.
- Navigation input may be consumed only while the future interactive overlay is
  visible.
- Unsupported firmware must fail closed without affecting the HTTP service.
- Hooks must be removed before unloading or replacing the injected module.

## Pull requests

- [x] PR 22: add the transport contract and testable overlay session model.
- [x] PR 23: embed and safely inject an inert overlay ELF into `SceShellUI`.
- [x] PR 24: add the controller shortcut and a non-interactive test panel.
- [x] PR 25: resolve the ShellUI notification entry point safely and add
  crash-boundary diagnostics.
- [ ] PR 26: render the cheat list and connect toggles to `CheatService`.
- [ ] PR 27: harden firmware profiles, rest-mode reinjection and performance.

Each step must leave the existing Media tile workflow operational. PRs 23 and
later require hardware validation before merge because host tests cannot prove
ShellUI ABI and firmware compatibility.

### PR 23 runtime diagnostics

The main payload starts injection on a detached thread. The inert module writes
the current ShellUI PID to `/system_tmp/blessing/overlay-ready`; a matching PID
prevents duplicate injection. Expected debug output is:

```text
Blessing overlay: inert module loaded pid=<pid> ready=yes
Blessing overlay: injection=ready pid=<pid>
```

Missing ShellUI, invalid embedded ELF, ptrace failures and readiness timeouts
are non-fatal. In every failure path the existing Media tile remains available.

### PR 24 shortcut probe

PR 24 observes the foreground user's controller at approximately 30 Hz. Holding
L3+R3 for one second toggles a native system notification card that acts as the
non-interactive test panel. The shortcut is armed again only after both buttons
are released, so one hold produces one transition.

This probe does not consume controller input, install PUI hooks, call the HTTP
API or change cheat state. The game may therefore also react to L3/R3. The
custom navigable panel and input ownership remain scoped to PR 25.

Once a controller handle is available, the module writes the ShellUI PID to
`/system_tmp/blessing/overlay-input-ready`. Expected debug output is:

```text
Blessing overlay: pad user=<user-id> handle=0x<handle>
Blessing overlay: input ready; hold L3+R3 for 1 second
Blessing overlay: test panel=open notify=0x0
Blessing overlay: test panel=closed notify=0x0
```

If controller reads repeatedly fail, the marker is removed and the module
reacquires the foreground controller without affecting the Media tile or cheat
service.

### PR 25 ShellUI notification hotfix

Hardware validation of PR 24 showed a brief ShellUI freeze and refresh when the
shortcut attempted to display the test card. PR 25 removes the direct
`sceKernelSendNotificationRequest` import. The overlay resolves that entry
point explicitly through `sceKernelDlsym` using the two known libkernel handles
and skips the notification if neither resolves. This follows the same boundary
used by OnionHEN's injected ShellUI module: a missing notification function is
non-fatal.

Expected debug output before the shortcut is accepted is:

```text
Blessing overlay: notification ready handle=0x<handle> address=<address>
Blessing overlay: input ready; hold L3+R3 for 1 second
```

The shortcut path logs `notification dispatch begin` before entering the
resolved function and `test panel=... notify=...` only after it returns. These
two messages make any remaining platform fault boundary unambiguous.

The injected module also reuses ShellUI's already initialized user service
instead of attempting to initialize that process-global service again.

All overlay diagnostics are mirrored to
`/data/ez-cheats/blessing-overlay.log`. The file is truncated when a new
overlay instance starts, so it contains only the current injected session and
cannot grow across restarts. Failure to create or append the file is non-fatal;
console output continues to work.

The hardware crash report captured a `SIGSEGV` on `SceShellUIMain` while PUI
was finishing the notification render (`UIRenderer.End` / `WaitVBlank`). The
notification request therefore uses the complete `0xC30` ABI layout and sets
`target_id=-1`, rather than relying on an opaque zero-filled 45-byte prefix.
