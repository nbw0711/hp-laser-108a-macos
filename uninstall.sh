#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
sudo -v
# Fails if jobs remain: the user can finish/cancel them in the print dialog first.
if lpstat -o HP_Laser_108a 2>/dev/null | /usr/bin/grep -q .; then
  echo 'Queue still contains jobs. Finish or cancel them before uninstalling.' >&2; exit 1
fi
sudo /bin/bash "$PWD/tools/restore-usb.sh"
if lpstat -p HP_Laser_108a >/dev/null 2>&1; then sudo lpadmin -x HP_Laser_108a; fi
sudo rm -f /usr/libexec/cups/backend/hp108a
sudo rm -f /Library/Printers/HP108a/rastertoqpdl /Library/Printers/HP108a/hp108a.ppd
sudo rmdir /Library/Printers/HP108a 2>/dev/null || true
printf 'Driver removed. Recorded bridge overrides restored; reconnect USB if needed.\n'
