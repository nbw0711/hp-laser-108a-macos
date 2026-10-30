# Validation

Test date: 2026-10-30 (Asia/Shanghai).

## Environment

HP Laser 108a, USB `03f0:e82a`, classic interface 0 / alternate setting 0. Host: M3 Max, macOS 27.0.1, system CUPS 2.3.4, Apple Command Line Tools.

## Offline checks passed

- Native arm64 and x86_64 Universal 2 compilation and linking, deployment target macOS 12.0. Only arm64 was executed.
- PPD consistency; build-time checks skip installed filter path/ownership checks. Full installed PPD validation also passed. The Oficio Adobe-name warning remains.
- Six synthetic raster cases: A4 three-page text, Letter sparse noise, Legal dense noise, A5 sparse noise, A4 two blank pages, and A4 with a two-copy field.
- SPL page headers, 0x11 decompression, checksums, end records, and decoded pixels in the printable region matched. Hardware margins may be clipped.
- Band widths: A4 4864 pixels, Letter/Legal 5120, A5 3328.
- Actual macOS cgpdftoraster pipeline: one-page PDF, three-page PDF, one-page PDF with two-copy metadata.
- PJL metadata escaping and absence of persistent PJL DEFAULT writes in the HP path.
- Shell syntax checks for build, install, uninstall, diagnostics and bridge helpers.

Run offline checks with `./build.sh && python3 tests/run.py`.

## Physical output confirmed

The first attempt stopped with exclusive-access error `0xe00002c5` while ippusbd held the USB device. After disabling and stopping only this printer's bridge and selecting the classic interface:

- A normal document and the single-page test printed correctly.
- The three-page A/B/C document printed correctly.
- Consecutive independent one-page jobs 1 and 2 printed correctly.
- The operator confirmed all five additional A/B/C/1/2 pages were present and correct, with no duplicates or missing pages.
- CUPS marked all these jobs completed; the queue returned idle with no new errors.

The bridge override is recorded and can be reversed by `tools/restore-usb.sh` or the uninstaller.

## Not yet hardware-tested

Intel hosts, older macOS releases, other printer models, other paper sizes, physical multiple-copy printing, sleep/wake, USB reconnect, and reboot recovery. No differential comparison against HP's proprietary encoder was performed.
