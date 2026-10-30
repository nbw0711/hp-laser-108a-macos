# HP Laser 108a for macOS

Native CUPS driver for the HP Laser 108a over USB, based on SpliX and the HP Laser 107a macOS project.

English | [简体中文](README.zh-CN.md)

**Hardware tested:** HP Laser 108a on an M3 Max running macOS 27.0.1. A normal document, a three-page document, and consecutive independent jobs printed correctly, with physical output confirmed. See [validation](VALIDATION.md) for the limits of this testing.

## Features

- Native Apple Silicon build; optional Universal 2 build for arm64 and x86_64.
- 600 dpi monochrome, simplex, A4 by default.
- A4, Letter, Legal, A5, A6, B5, Executive, Folio and Oficio options in the PPD. Physical testing used A4.
- Uses macOS's PDF rasterizer, a SpliX SPL/QPDL encoder, and an IOKit USB backend. No Windows VM or Ghostscript required.
- Matches USB device `03f0:e82a`, shared by the HP Laser 103/107/108 family. Only the 108a was hardware-tested here. Connect one matching printer at a time.

## Install

Requires macOS, Apple's Command Line Tools, and administrator access. Python 3 is used by offline tests only.

```sh
xcode-select --install  # if Command Line Tools are not already installed

git clone https://github.com/nbw0711/hp-laser-108a-macos.git
cd hp-laser-108a-macos
./install.sh
```

The installer builds from source and creates the queue `HP_Laser_108a` (shown as **HP Laser 108a**). Select it in any application's print dialog. It does not change your default printer.

Installed files:

- `/Library/Printers/HP108a/rastertoqpdl`
- `/Library/Printers/HP108a/hp108a.ppd`
- `/usr/libexec/cups/backend/hp108a`

If macOS refuses the backend installation path, installation stops. There is no network-daemon fallback.

Test a single page first:

```sh
lp -d HP_Laser_108a tests/testpage.pdf
```

## USB device busy / queue paused

On the tested machine, macOS's IPP-over-USB bridge held the device exclusively (`0xe00002c5`). The driver uses its classic USB print interface instead.

Inspect the device and find its exact bridge label:

```sh
./diagnose.sh
sudo ./build/hp108a --probe
sudo launchctl list | grep -i 'com.apple.print.ippusb.*HP.*Laser'
```

Only after confirming the label belongs to your HP Laser 108a, pass the **complete label**, including spaces, to:

```sh
sudo bash tools/prepare-usb.sh 'com.apple.print.ippusb.HP Inc..HP Laser 108a.YOUR_SERIAL'
sudo cupsenable HP_Laser_108a
```

This persistently disables and stops **that device's** IPP-over-USB bridge. The helper records the changed label under `/Library/Printers/HP108a/`; the classic USB backend handles printing. The helper accepts only HP Laser 108a labels and does not kill system processes or reset the printer.

To restore the recorded bridge configuration:

```sh
sudo bash tools/restore-usb.sh
```

Reconnect USB after restoring. Do not disable another printer's bridge. A VM holding the USB device can also cause an exclusive-access error. Check physical output before resuming a failed job to avoid duplicate pages.

## Build and offline tests

```sh
./build.sh
python3 tests/run.py

# Optional: build both architectures
ARCHS='arm64 x86_64' ./build.sh
```

Tests exercise raster-to-SPL checksums and decoded pixels, page counts, copy fields, band widths, and the actual macOS PDF-to-raster pipeline. They do not install anything or send pages to a printer.

The binary deployment target is macOS 12.0; this is not a claim of hardware testing on every intervening macOS release. Intel is compile-tested only.

## Uninstall

Finish or cancel jobs in this queue, then:

```sh
./uninstall.sh
```

The uninstaller removes this queue and driver files and re-enables bridge labels recorded by the helper. Reconnect USB afterward. Unrecorded manual service changes must be restored separately.

## Limitations

This is an unsigned community driver using the macOS CUPS/PPD interface. Sleep/wake, USB reconnect, reboot recovery, other paper sizes and physical multiple-copy output remain untested. No 1200 dpi option, automatic duplex, scanning, toner status, AirPrint or network printing is provided. A successful USB transfer alone does not prove physical output.

## Credits and license

This adaptation builds on:

- [paviladtudk/hp-laser-107a-macos](https://github.com/paviladtudk/hp-laser-107a-macos): the vendored SpliX tree, PPD, test tools and USB transport starting point.
- [OpenPrinting SpliX](https://github.com/OpenPrinting/splix): SPL/QPDL encoding, including HP Laser 10x band-width support.
- [Kuberwastaken/hp-laser-1008a-macos](https://github.com/Kuberwastaken/hp-laser-1008a-macos): MIT-licensed IOKit transport ancestry.

GPL-2.0 applies to SpliX and GPL-derived portions; the USB transport retains its MIT ancestry and notice. See [LICENSE](LICENSE), [third-party notices](THIRD_PARTY.md), and `vendor/LICENSE-usb-MIT`. Original copyright notices are retained. The original upstream source snapshot and hashes are included for provenance.

No HP Windows installer, DLL, firmware or proprietary driver code is distributed in this repository. Not affiliated with HP or Samsung.
