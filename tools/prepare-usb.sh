#!/bin/bash
# Reviewed preparation only: run after explicit authorization to change the service.
# Usage: sudo bash tools/prepare-usb.sh 'complete observed launchd label'
set -euo pipefail
[[ $EUID -eq 0 ]] || { echo 'Administrator privileges required' >&2; exit 1; }
label=${1:?Provide the complete observed HP Laser 108a launchd label}
case "$label" in
  'com.apple.print.ippusb.HP Inc..HP Laser 108a.'?*) ;;
  *) echo 'Refusing a label outside the HP Laser 108a device scope' >&2; exit 1;;
esac
state=/Library/Printers/HP108a/ippusb-disabled-labels.txt
[[ -d /Library/Printers/HP108a ]] || { echo 'Install driver first' >&2; exit 1; }
if ! /bin/launchctl print-disabled system | /usr/bin/grep -F "\"$label\" =>" | /usr/bin/grep -Eq '=> (true|disabled)'; then
  # Record exactly the enabled device bridge changed by this installation.
  printf '%s\n' "$label" >> "$state"
  /bin/launchctl disable "system/$label"
fi
if /bin/launchctl print "system/$label" >/dev/null 2>&1; then
  /bin/launchctl bootout "system/$label"
fi
/usr/libexec/cups/backend/hp108a --probe
