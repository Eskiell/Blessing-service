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
- Controller input is consumed only while the overlay is visible.
- No polling is performed while the overlay is closed.
- Unsupported firmware must fail closed without affecting the HTTP service.
- Hooks must be removed before unloading or replacing the injected module.

## Pull requests

- [x] PR 22: add the transport contract and testable overlay session model.
- [x] PR 23: embed and safely inject an inert overlay ELF into `SceShellUI`.
- [ ] PR 24: add the controller shortcut and a non-interactive test panel.
- [ ] PR 25: render the cheat list and connect toggles to `CheatService`.
- [ ] PR 26: harden firmware profiles, rest-mode reinjection and performance.

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
