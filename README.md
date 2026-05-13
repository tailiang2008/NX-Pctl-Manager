# NX-Pctl-Manager

[![build](https://github.com/tailiang2008/NX-Pctl-Manager/actions/workflows/build.yml/badge.svg)](https://github.com/tailiang2008/NX-Pctl-Manager/actions/workflows/build.yml)
[![license: GPLv3](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE)
[![latest release](https://img.shields.io/github/v/release/tailiang2008/NX-Pctl-Manager)](https://github.com/tailiang2008/NX-Pctl-Manager/releases/latest)

A Nintendo Switch parental-controls tool — **no phone app, no Nintendo account, no internet** — configure the daily play-time limit directly on the console, and reset / delete the PIN or unlink the companion app while you're at it.

> ⚠️ **Requires custom firmware (CFW; Atmosphère recommended).** It calls the restricted system `pctl` service directly, so it only runs on a hacked console (a stock retail console can't use it). It doesn't bypass any account or online verification — it just brings the parental-controls settings that are otherwise buried in the phone app / deep in System Settings (plus the forgot-PIN recovery options) onto the console, for CFW users to do offline.
>
> It talks to restricted / `*ForDebug` `pctl` commands directly — usually fine, but in a few states (notably *removing* the play-time limit while the "time's up" lock screen is up) it can destabilise Atmosphère. **Tested on firmware 22.1.0 / Atmosphère 1.11.1; other versions are unverified. Use at your own risk.**

---

## Features

### Configure the play timer (daily limit) offline — the main point of this tool

Nintendo **only lets you set the daily play-time limit through the "Nintendo Switch Parental Controls" phone app** and sync it to the console; there's no option for it in the console's System Settings. This tool talks to the console's `pctl` service directly, so **on the console, fully offline** (no phone app, no Nintendo account, no internet) you can:

- **Set one limit for every day** — one minute value applied to Sunday–Saturday.
- **Set each day separately** — Sunday, Monday … Saturday each get their own value (edited day-by-day in a sub-menu, staged, then saved all at once).
- **Turn the limit off** — remove the play timer entirely.

Notes: `0` minutes means that day is **fully blocked** (not "no limit"); for "no limit", use *Remove play-time limit* to turn the whole timer off. Once set, the play timer takes effect when you launch a game (verified on fw 22.1.0 / Atmosphère 1.11.1).

> If the play timer is **currently active** (already counting down), overwriting its config directly destabilises Atmosphère — so the tool first turns parental controls off temporarily (it reads and uses the console's PIN automatically — **you don't need to remember it**), writes the new value, and you return to the main menu; the new limit takes effect once parental controls are active again.

### Other (parental-controls recovery / maintenance)

- **Set / change the PIN** — opens the system's parental-controls passcode screen.
- **Delete all parental controls** — wipes the PIN and every restriction (**irreversible**; confirmed before it runs). Your recovery option when you've forgotten the PIN.
- **Unlink the companion app** — breaks the link between the "Nintendo Switch Parental Controls" phone app and this console.
- **View status** — current safety level, whether a PIN is set (and its length), whether restrictions are enabled, and play-timer status (active or not / the configured daily limit).

## How to use

After launching from hbmenu: ↑ / ↓ move the cursor, (A) confirms, (B) goes back one level / (on the main menu) exits. Destructive actions (delete all parental controls, unlink companion app, remove the limit) and "turn parental controls off temporarily" all show a confirmation screen first.

1. **(Recommended first step) Unlink the companion app** — main menu → *Unlink companion app*. If the phone parental-controls app is still linked to this console, its next sync will overwrite the limit you set on the console.
2. **Set the play timer** — main menu → *Play timer (daily limit)*:
   - *Set daily limit (all days)*: pops a number pad for the minutes (0–1440), applies it to every day, writes it after you confirm.
   - *Per-day limits*: opens a sub-menu; press (A) on a day to type its value (it's **staged** — edited days are marked `(*)`); when you're done, pick *Save per-day limits* to write all 7 at once; press (B) to leave without saving.
   - *Remove play-time limit*: turns the whole timer off (with confirmation).
   - When writing, if the timer is active you'll first get a confirmation → (once you agree) parental controls are turned off temporarily → the new value is written → you're told to return to the main menu (the new limit takes effect once parental controls are active again). ⚠️ **Don't set the limit below "time already played today"** — the moment parental controls come back on, it locks immediately.
3. **Change / delete the PIN** — *Set / change parental control PIN* switches to the system applet to set a passcode and returns automatically when done; *Delete all parental controls* wipes the PIN and every restriction (with confirmation, **irreversible**).
4. **View / refresh status** — the main menu always shows a status panel; *Refresh status* re-reads it.

## Install

Grab `nx_pctl_manager.nro` from the [**Releases**](../../releases) page, drop it in `/switch/` on the SD card, and run it from hbmenu. (Or download `nx_pctl_manager.zip` from the same release and unzip it onto the SD card root — it puts the `.nro` in `/switch/` for you.)

## Build from source

Needs [devkitPro](https://devkitpro.org/) and the `switch-dev` toolchain (libnx included):

```sh
export DEVKITPRO=/opt/devkitpro
make            # produces ./nx_pctl_manager.nro
make dist       # packs nx_pctl_manager.zip — unzip it straight onto the SD card
make nxlink     # build and push over nxlink to a Switch running hbmenu on the LAN
make clean
```

Without a native toolchain, the repo's `./run.sh` builds via the official Docker image `devkitpro/devkita64`: `./run.sh` just builds; `./run.sh <switch-ip>` builds then pushes over nxlink (press Y in hbmenu first).

## License

GPLv3 (see [`LICENSE`](LICENSE)).
