#!/bin/bash
set -u
cd "$(dirname "$0")"
sw_vers
uname -m
lpstat -t
/usr/sbin/ioreg -r -c IOUSBHostDevice -l -w 0 | /usr/bin/grep -E '(^[ |+`-]*\+-o |"(idVendor|idProduct|USB Product Name)" =)' || true
launchctl list | /usr/bin/grep -i 'com.apple.print.ippusb.*HP.*Laser' || true
if [[ -x build/hp108a ]]; then build/hp108a --probe; fi
