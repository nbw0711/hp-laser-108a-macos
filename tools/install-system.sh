#!/bin/bash
# Privileged installation phase. Invoke only after building and validating locally.
set -euo pipefail
cd "$(dirname "$0")/.."
[[ $EUID -eq 0 ]] || { echo 'Administrator privileges required' >&2; exit 1; }
if /usr/bin/lpstat -p HP_Laser_108a >/dev/null 2>&1; then
  echo 'HP_Laser_108a already exists; installation stopped.' >&2; exit 1
fi
/usr/bin/install -m 700 -o root -g wheel build/hp108a /usr/libexec/cups/backend/hp108a
/usr/bin/install -d -m 755 -o root -g wheel /Library/Printers/HP108a
/usr/bin/install -m 755 -o root -g wheel build/rastertoqpdl /Library/Printers/HP108a/rastertoqpdl
/usr/bin/install -m 644 -o root -g wheel ppd/hp108a.ppd /Library/Printers/HP108a/hp108a.ppd
/usr/sbin/lpadmin -p HP_Laser_108a -E -v hp108a:/ \
  -P /Library/Printers/HP108a/hp108a.ppd -D 'HP Laser 108a' -L USB \
  -o printer-is-shared=false -o printer-error-policy=stop-printer -o PageSize=A4
/usr/bin/cupstestppd -q /Library/Printers/HP108a/hp108a.ppd
/usr/bin/lpstat -v HP_Laser_108a
