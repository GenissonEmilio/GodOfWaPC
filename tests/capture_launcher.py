#!/usr/bin/env python3
"""Render the real launcher and report native startup/loader failures."""
import argparse
import os
from pathlib import Path
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runner',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args()
runner=a.runner.resolve(strict=True)
output=a.output.resolve()
output.unlink(missing_ok=True)
result=subprocess.run([str(runner),'--capture-launcher',str(output)],
                      env=dict(os.environ,SDL_VIDEODRIVER='dummy'),
                      stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
print(result.stdout.decode('utf-8',errors='replace'),end='')
print(f'Launcher exit: {result.returncode} / 0x{result.returncode & 0xFFFFFFFF:08X}')
if result.returncode and os.name=='nt':
    import ctypes
    with os.add_dll_directory(str(runner.parent)):
        for name in ('libc++.dll','libunwind.dll','libwinpthread-1.dll','SDL2.dll','wgpu_native.dll'):
            try:
                ctypes.WinDLL(str(runner.parent/name))
                print(name,'loads successfully')
            except OSError as error:
                print(name,'LOAD FAILED:',error)
if result.returncode or not output.is_file() or output.stat().st_size<100000:
    raise SystemExit('Launcher did not produce a valid capture')
with output.open('rb') as image:
    if image.read(2)!=b'BM':
        raise SystemExit('Unexpected screenshot format')
print('Launcher render: PASS')
