# Contributing

For a bug report, include the macOS version, chip architecture, exact printer model, paper size, and whether the failure occurs before or after USB transfer. Describe physical output as well as CUPS status.

Run `./diagnose.sh` locally. Redact serial numbers, usernames, document titles and personal paths before sharing output. Do not upload proprietary Windows drivers or documents containing personal information.

For code changes, run `./build.sh`, `python3 tests/run.py` and shell syntax checks for changed scripts. State which checks were run and which printer/OS combinations were physically tested. Offline checks do not establish physical printing compatibility.

Preserve third-party copyright notices and update THIRD_PARTY.md for changes to vendored sources.
