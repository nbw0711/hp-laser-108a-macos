#!/usr/bin/env python3
"""Offline round-trip tests: synthetic CUPS raster -> SPL -> decoded pixels."""
from pathlib import Path
import importlib.util
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('splcheck', root / 'tests/splcheck.py')
check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check)
ppd = root / 'ppd/hp108a.ppd'

def run(args, **kwargs):
    return subprocess.run([str(a) for a in args], check=True, timeout=60, **kwargs)

cases = [('A4', 'text', 3, 1, 4864), ('Letter', 'noise', 1, 1, 5120),
         ('Legal', 'dense', 1, 1, 5120), ('A5', 'noise', 1, 1, 3328),
         ('A4', 'blank', 2, 1, None), ('A4', 'noise', 1, 2, 4864)]
with tempfile.TemporaryDirectory(prefix='hp108a-test-') as td:
    td = Path(td)
    for idx, (paper, pattern, count, copies, width) in enumerate(cases):
        ras, spl, expected = [td / f'{idx}.{ext}' for ext in ('ras', 'spl', 'expected')]
        opts = f'PageSize={paper} Resolution=600dpi copies={copies}'
        with ras.open('wb') as out:
            result = run([root / 'build/mkraster', ppd, opts, pattern, count, expected],
                         stdout=out, stderr=subprocess.PIPE)
        bpl = int(re.search(rb'bpl=(\d+)', result.stderr).group(1))
        with spl.open('wb') as out:
            run([root / 'build/rastertoqpdl', '1', 'test\r\n@PJL RESET', 'Test "metadata"\x1b',
                 str(copies), opts, ras], env={**os.environ, 'PPD': str(ppd)},
                stdout=out, stderr=subprocess.PIPE)
        raw = spl.read_bytes()
        assert b'@PJL DEFAULT' not in raw, 'persistent printer setting in job'
        assert b'@PJL RESET' not in raw.split(b'\r\n'), 'PJL metadata injection'
        pages, errors = check.parse(spl)
        assert not errors, errors
        assert len(pages) == count, (len(pages), count)
        for page_idx, page in enumerate(pages):
            assert page.get('end') and page['res'] == 600 and page['copies'] == copies, {k: v for k, v in page.items() if k != 'bands'}
            widths = {band[0] for band in page['bands'].values()}
            if width is not None:
                assert widths == {width}, widths
            comparison = check.compare(page, f'{expected}.p{page_idx+1}', bpl)
            assert 'FAIL' not in comparison, comparison
        print(f'PASS {paper} {pattern}: {count} page(s), {copies} copies, checksums + decoded pixels', flush=True)
# Device enumeration must not invent an available printer when no device is connected.
result = run([root / 'build/hp108a'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if result.stdout:
    assert result.stdout.startswith(b'direct hp108a:/ '), result.stdout
print('PASS USB backend discovery contract')
# Exercise the real macOS PDF renderer as well as the synthetic raster generator.
with tempfile.TemporaryDirectory(prefix='hp108a-pdf-') as td:
    td = Path(td)
    for name, count, copies in [('testpage.pdf', 1, 1), ('three-pages.pdf', 3, 1), ('testpage.pdf', 1, 2)]:
        ras, spl = td / 'pdf.ras', td / 'pdf.spl'
        with ras.open('wb') as out:
            run(['/usr/sbin/cupsfilter', '-p', ppd, '-n', str(copies), '-m', 'application/vnd.cups-raster',
                 root / 'tests' / name], stdout=out, stderr=subprocess.PIPE)
        assert ras.stat().st_size > 0
        with spl.open('wb') as out:
            run([root / 'build/rastertoqpdl', '2', 'test', name, str(copies), 'PageSize=A4', ras],
                env={**os.environ, 'PPD': str(ppd)}, stdout=out, stderr=subprocess.PIPE)
        pages, errors = check.parse(spl)
        assert not errors and len(pages) == count, (errors, len(pages))
        for page in pages:
            assert page.get('end') and page['bands']
            assert page['copies'] == copies, (page['copies'], copies)
            assert {band[0] for band in page['bands'].values()} == {4864}
        print(f'PASS macOS cgpdftoraster -> SPL: {name}, {count} page(s), {copies} copies', flush=True)
