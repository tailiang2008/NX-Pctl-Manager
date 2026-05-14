# Changelog

## v3.0.0

UI rewrite onto [borealis](https://github.com/xfangfang/borealis) — Horizon-system-style graphical UI replaces the v2 text console. **No functional changes** — every action behaves the same.

- App display name is now **"Pctl Manager"** (repo name `NX-Pctl-Manager` unchanged).
- Non-CFW boot shows a dedicated error screen (was a silent fall-through in v2).
- Build switched to CMake; `make` / `./run.sh` interfaces unchanged.
- `.nro` grew 265 KB → ~8.2 MB (borealis library + assets).

Tested on fw 22.1.0 / Atmosphère 1.11.1. `make PROBE=1` still adds the diagnostic dump cell.

## v2.0.0

First public release. CFW (Atmosphère) only; tested on fw 22.1.0 / Atmosphère 1.11.1.

- **Configure the daily play-time limit offline** — one for all days, per-day limits, or off. When the timer is active, writes turn parental controls off temporarily (the app reads the PIN automatically) so the write doesn't destabilise Atmosphère.
- Set / change PIN via the system passcode applet.
- Delete all parental controls — also the recovery path when the PIN is forgotten.
- Unlink the Nintendo Switch Parental Controls companion phone app.
- View status: safety level, PIN length, restrictions, play-timer state.

`make PROBE=1` adds a read-only "Dump current config" diagnostic cell (off in release builds).
