#!/usr/bin/env python3
"""Package a locally compiled game; never copies the ISO or extracted ELF."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', required=True, type=Path)
    a = p.parse_args()
    build = a.build.resolve()
    windows = (build / 'ps2run.exe').is_file()
    target = 'Windows' if windows else 'Linux'
    binary = build / ('ps2run.exe' if windows else 'ps2run')
    if not binary.is_file():
        p.error('Executavel compilado ausente.')
    report_path = build / 'build-report.json'
    if not report_path.is_file():
        p.error('build-report.json ausente. Gere o relatorio para esta compilacao.')
    report = json.loads(report_path.read_text(encoding='utf-8'))
    if report.get('binary_sha256') != hashlib.sha256(binary.read_bytes()).hexdigest():
        p.error('O relatorio nao corresponde ao executavel. Atualize os testes e o relatorio antes de empacotar.')
    out = ROOT / f'dist/GodOfWarII-{target}'
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binary, out / ('GodOfWarII.exe' if windows else 'GodOfWarII.bin'))
    shutil.copytree(build / 'fonts', out / 'fonts', dirs_exist_ok=True)
    if windows:
        dll_names = {'SDL2.dll', 'wgpu_native.dll', 'libc++.dll', 'libunwind.dll', 'libwinpthread-1.dll'}
        for dll in (build / name for name in dll_names if (build / name).exists()):
            shutil.copy2(dll, out / dll.name)
    else:
        libs = out / 'lib'
        libs.mkdir(exist_ok=True)
        dependencies = subprocess.check_output(['ldd', str(binary)], text=True)
        for line in dependencies.splitlines():
            match = re.search(r'(libc\+\+[^ ]*|libc\+\+abi[^ ]*|libunwind[^ ]*) => (\S+)', line)
            if match:
                shutil.copy2(match[2], libs / match[1])
        wrapper = out / 'GodOfWarII.sh'
        wrapper.write_text('#!/bin/sh\nset -eu\nHERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$HERE/GodOfWarII.bin" "$@"\n')
        wrapper.chmod(0o755)
    for name in ('README-PORT.md', 'VALIDACAO.md', 'CREDITOS.md', 'LICENSE'):
        if (ROOT / name).is_file():
            shutil.copy2(ROOT / name, out / name)
    (out / 'docs').mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'docs/UPSTREAM.md', out / 'docs/UPSTREAM.md')
    notices = out / 'licenses'
    notices.mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'third_party/imgui/LICENSE.txt', notices / 'Dear-ImGui.txt')
    for notice in (ROOT / 'third_party/notices').glob('*'):
        shutil.copy2(notice, notices / notice.name)
    sdl_license = ROOT / 'tools/SDL2-2.32.10/COPYING.txt'
    if sdl_license.exists():
        shutil.copy2(sdl_license, notices / 'SDL2.txt')
    for name in ('build-report.json',):
        if (build / name).exists():
            shutil.copy2(build / name, out / name)
    hashes = []
    for file in sorted(out.rglob('*')):
        if file.is_file() and file.name != 'SHA256SUMS.txt':
            hashes.append(hashlib.sha256(file.read_bytes()).hexdigest() + '  ' + file.relative_to(out).as_posix())
    (out / 'SHA256SUMS.txt').write_text('\n'.join(hashes) + '\n')
    if windows:
        archive = Path(shutil.make_archive(str(out), 'zip', out.parent, out.name))
    else:
        archive = out.with_suffix('.tar.gz')
        def permissions(info):
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mode = 0o755 if info.isdir() or info.name.endswith(('.sh', '.bin')) else 0o644
            return info
        with tarfile.open(archive, 'w:gz') as tar:
            tar.add(out, arcname=out.name, filter=permissions)
    archive.with_name(archive.name + '.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest() + '  ' + archive.name + '\n')
    print(out)
    print(archive)


if __name__ == '__main__':
    main()
