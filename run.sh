#!/usr/bin/env sh
# run.sh — build NX-Pctl-Manager in the devkitPro Docker image, and optionally
# push it to a Switch running hbmenu's netloader (in hbmenu, press Y first).
#
# Usage:
#   ./run.sh                  build only -> ./nx_pctl_manager.nro
#   ./run.sh <switch-ip>      build, then nxlink to that IP and stream stdout back
#   ./run.sh -a <switch-ip>   same
#   ./run.sh clean            run `make clean` in the container
#   PROBE=1 ./run.sh [...]    extra "Dump current config" diagnostic menu item
#
# Needs Docker. The devkitpro/devkita64 image is pulled automatically on first use.
# (Build artifacts stay owned by the current user; nothing is written to /opt.)

set -e

IMG=devkitpro/devkita64
DIR=$(cd "$(dirname "$0")" && pwd)

# Forward PROBE through to `make` in the container, so `PROBE=1 ./run.sh ...`
# builds the diagnostic dump cell into the Play timer menu.
dkp() {
    docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp -e PROBE="${PROBE:-}" \
        -v "$DIR":/project -w /project "$@"
}

if [ "$1" = "clean" ]; then
    dkp "$IMG" make clean
    exit 0
fi

echo ">> building (devkitpro/devkita64${PROBE:+, PROBE=$PROBE})..."
dkp "$IMG" make

ip=$1
[ "$ip" = "-a" ] && ip=$2

if [ -z "$ip" ]; then
    echo ">> built ./nx_pctl_manager.nro  (no Switch IP given — skipping nxlink)"
    echo "   to push:  ./run.sh <switch-ip>     (open hbmenu and press Y first)"
    exit 0
fi

echo ">> nxlink -> $ip   (press Y in hbmenu if you haven't; Ctrl-C to stop the log)"
# --network host lets the container (OrbStack's VM) reach the Switch on the LAN
# and receive the stdout/stderr stream back.
dkp --network host "$IMG" nxlink -s -a "$ip" nx_pctl_manager.nro
