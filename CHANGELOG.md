# Changelog

## v3.0.0

UI rewrite onto [borealis](https://github.com/xfangfang/borealis) — the app now renders a Horizon-system-style graphical UI (rounded cards, focus animations, native footer button hints, auto-themed dark/light) instead of the v2 text console. Functionality is unchanged: every action and flow maps one-to-one to the v2 menus.

- Main menu: status panel (PIN / Restrictions enabled / Parental control enabled) above five action cells.
- Play timer sub-menu: state header (Limit reached today / Today's remaining / Configured limit) above three actions — set daily limit, per-day limits, remove play-time limit.
- Per-day sub-menu: seven day rows with `(*)` markers for staged edits, plus a Save row that writes all seven at once (B-back discards staged edits).
- Destructive actions (delete-all / unlink / remove limit) and the write-before-temporary-unlock gate all use native `brls::Dialog` confirms.
- Hbmenu launcher label and the in-app title are now **"Pctl Manager"** (the repo / package name `NX-Pctl-Manager` is unchanged).
- The system `swkbd` number pad (play-timer minutes) and `pctlauth` passcode applet are unchanged — they already look native.
- An *init-error* screen replaces the silent fall-through when `pctl_ops_init` fails (running on a stock non-CFW console): the error code is shown, B exits the app.

Internal changes worth noting: the C service layer `source/pctl_ops.{h,c}` is untouched between v2 and v3 (same wire-protocol, same fw-22.1.0 verified behaviour); everything else under `source/` is new C++ on top of borealis. Build is now CMake-driven (the `Makefile` is a thin shim so `make` / `make PROBE=1` / `make dist` / `./run.sh <ip>` keep working). `.nro` size grows from 265 KB to ~8.2 MB (borealis library + Material icon font + built-in widget resources).

Requires CFW (Atmosphère recommended). Tested on firmware 22.1.0 / Atmosphère 1.11.1.

`make PROBE=1` keeps working and adds the read-only "Dump current config" diagnostic cell to the Play timer sub-menu (omitted from normal builds and the release `.nro`).

## v2.0.0

First public release.

- **Configure the daily play-time limit offline** — on the console, with no companion phone app, no Nintendo account, and no internet: one limit for all days, per-day limits (staged then saved together), or turn the timer off. When the timer is already active, a write first turns parental controls off temporarily (it reads and uses the console's PIN automatically — you don't need to remember it), so it doesn't destabilise Atmosphère; the new limit takes effect once parental controls re-engage.
- **Set / change the parental-controls PIN** — via the system passcode applet.
- **Delete all parental controls** — wipes the PIN and every restriction (irreversible). The recovery option when the PIN has been forgotten.
- **Unlink the companion app** — breaks the link between the "Nintendo Switch Parental Controls" phone app and the console.
- **View status** — safety level, whether a PIN is set (and its length), whether restrictions are enabled, and the play-timer state / configured daily limit.

Requires CFW (Atmosphère recommended). Tested on firmware 22.1.0 / Atmosphère 1.11.1.

`make PROBE=1` adds a read-only "Dump current config" diagnostic item to the play-timer menu (off in normal builds and in the release `.nro`).
