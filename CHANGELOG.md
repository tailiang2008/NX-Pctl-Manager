# Changelog

## v2.0.0

First public release.

- **Configure the daily play-time limit offline** — on the console, with no companion phone app, no Nintendo account, and no internet: one limit for all days, per-day limits (staged then saved together), or turn the timer off. When the timer is already active, a write first turns parental controls off temporarily (it reads and uses the console's PIN automatically — you don't need to remember it), so it doesn't destabilise Atmosphère; the new limit takes effect once parental controls re-engage.
- **Set / change the parental-controls PIN** — via the system passcode applet.
- **Delete all parental controls** — wipes the PIN and every restriction (irreversible). The recovery option when the PIN has been forgotten.
- **Unlink the companion app** — breaks the link between the "Nintendo Switch Parental Controls" phone app and the console.
- **View status** — safety level, whether a PIN is set (and its length), whether restrictions are enabled, and the play-timer state / configured daily limit.

Requires CFW (Atmosphère recommended). Tested on firmware 22.1.0 / Atmosphère 1.11.1.

`make PROBE=1` adds a read-only "Dump current config" diagnostic item to the play-timer menu (off in normal builds and in the release `.nro`).
