#!/bin/bash
# Undo only bridge overrides recorded by this HP108a installation.
set -euo pipefail
[[ $EUID -eq 0 ]] || { echo 'Administrator privileges required' >&2; exit 1; }
state=/Library/Printers/HP108a/ippusb-disabled-labels.txt
[[ -f "$state" ]] || exit 0
while IFS= read -r label; do
  [[ -n "$label" ]] || continue
  case "$label" in
    'com.apple.print.ippusb.HP Inc..HP Laser 108a.'?*) ;;
    *) echo 'Unexpected device label; restore stopped.' >&2; exit 1;;
  esac
  /bin/launchctl enable "system/$label"
done < "$state"
/bin/rm -f "$state"
printf 'Recorded HP108a IPP-USB overrides restored. Reconnect USB to restart the bridge.\n'
