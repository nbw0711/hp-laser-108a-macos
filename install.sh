#!/bin/bash
# Installs only the HP108a files and a dedicated CUPS queue.
set -euo pipefail
cd "$(dirname "$0")"
./build.sh
if lpstat -p HP_Laser_108a >/dev/null 2>&1; then
  echo 'Queue HP_Laser_108a already exists. Remove it with uninstall.sh before reinstalling.' >&2
  exit 1
fi
sudo -v
sudo install -d -m 755 -o root -g wheel /Library/Printers/HP108a
sudo install -m 755 -o root -g wheel build/rastertoqpdl /Library/Printers/HP108a/rastertoqpdl
sudo install -m 644 -o root -g wheel ppd/hp108a.ppd /Library/Printers/HP108a/hp108a.ppd
sudo install -m 700 -o root -g wheel build/hp108a /usr/libexec/cups/backend/hp108a
sudo lpadmin -p HP_Laser_108a -E -v hp108a:/ \
  -P /Library/Printers/HP108a/hp108a.ppd -D 'HP Laser 108a' -L USB \
  -o printer-is-shared=false -o printer-error-policy=stop-printer -o PageSize=A4
printf '\nInstalled HP_Laser_108a. Test: lp -d HP_Laser_108a tests/testpage.pdf\n'
printf 'If USB is busy, run ./diagnose.sh and consult README.md.\n'
