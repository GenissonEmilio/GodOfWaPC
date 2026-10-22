#!/usr/bin/env python3
"""Build God of War II from the user's USA DVD, on Windows or Linux.

No game or generated code is fetched. Both binaries are compiled from the
same locally generated C++ and read game data straight from the original ISO.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from disc import Image

ELF_SHA256 = 'fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6'
IS_WINDOWS = os.name == 'nt'
SUFFIX = '.exe' if IS_WINDOWS else ''


def run(args, **kwargs):
    print('+', ' '.join(map(str, args)), flush=True)
    return subprocess.run(list(map(str, args)), cwd=ROOT, check=True, **kwargs)


def tool(name, candidates=()):
    for candidate in candidates:
        if Path(candidate).is_file():
            return str(Path(candidate).resolve())
    result = shutil.which(name)
    if not result:
        raise RuntimeError(f'{name} ausente. Consulte README-PORT.md para preparar as dependencias.')
    return result


def download_zip(url, destination):
    destination.mkdir(parents=True, exist_ok=True)
    temporary = destination / 'download.zip'
    print('Download:', url, flush=True)
    urllib.request.urlretrieve(url, temporary)
    with zipfile.ZipFile(temporary) as archive:
        archive.extractall(destination)
    temporary.unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--frames', type=int, default=12000)
    parser.add_argument('--rounds', type=int, default=12)
    parser.add_argument('--no-explore', action='store_true')
    parser.add_argument('--package', action='store_true')
    args = parser.parse_args()
    iso = args.iso.resolve(strict=True)
    if iso.suffix.lower() != '.iso':
        parser.error('Forneca a ISO, nao o patch xdelta ou seu aplicador.')
    game = ROOT / 'games/gow2'
    disc = game / 'disc'
    generated = ROOT / 'generated/gow2'
    for directory in (disc, generated, game / 'vu1', game / 'shots'):
        directory.mkdir(parents=True, exist_ok=True)
    image = Image(iso)
    try:
        entries = list(image.files())
        for name in ('SYSTEM.CNF', 'SCUS_974.81'):
            entry = next((e for e in entries if e[0] == name), None)
            if not entry:
                raise RuntimeError(f'{name} ausente: a ISO nao e a versao USA esperada.')
            image.copy(entry, str(disc / name))
        if len(image.layers) != 2 or not image.find('PART2.PAK'):
            raise RuntimeError('A segunda camada do DVD nao foi reconhecida.')
    finally:
        image.file.close()
    elf = disc / 'SCUS_974.81'
    sha = hashlib.sha256(elf.read_bytes()).hexdigest()
    if sha != ELF_SHA256:
        raise RuntimeError(f'Executavel diferente da versao suportada. SHA256: {sha}')

    cmake = tool('cmake', [ROOT / 'tools/python/cmake/data/bin/cmake.exe'] if IS_WINDOWS else [])
    ninja = tool('ninja', [ROOT / 'tools/python/bin/ninja.exe'] if IS_WINDOWS else [])
    flags = []
    if IS_WINDOWS:
        compilers = sorted((ROOT / 'tools').glob('llvm-mingw-*-ucrt-x86_64/bin/x86_64-w64-mingw32-clang++.exe'))
        cxx = tool('clang++', compilers[-1:] if compilers else [])
        os.environ['PATH'] = str(Path(cxx).parent) + os.pathsep + os.environ['PATH']
        sdl = ROOT / 'tools/SDL2-2.32.10/x86_64-w64-mingw32'
        if not sdl.is_dir():
            raise RuntimeError('SDL2 ausente. Execute scripts/setup_windows.ps1.')
        flags += [f'-DCMAKE_PREFIX_PATH={sdl}']
        wgpu = ROOT / 'third_party/wgpu-native-windows'
        library = wgpu / 'lib/libwgpu_native.dll.a'
        if not library.exists():
            download_zip('https://github.com/gfx-rs/wgpu-native/releases/download/v29.0.1.1/wgpu-windows-x86_64-gnu-release.zip', wgpu)
    else:
        cxx = tool('clang++-18') if shutil.which('clang++-18') else tool('clang++')
        flags += ['-DCMAKE_CXX_FLAGS=-stdlib=libc++']
        wgpu = ROOT / 'third_party/wgpu-native'
        library = wgpu / 'lib/libwgpu_native.a'
        if not library.exists():
            architecture = 'aarch64' if platform.machine() in ('aarch64', 'arm64') else 'x86_64'
            download_zip(f'https://github.com/gfx-rs/wgpu-native/releases/download/v29.0.1.1/wgpu-linux-{architecture}-release.zip', wgpu)
    flags += [f'-DCMAKE_CXX_COMPILER={cxx}', f'-DCMAKE_MAKE_PROGRAM={ninja}',
              f'-DPS2_WGPU_DIR={wgpu}', f'-DPS2_WGPU_LIBRARY={library}']
    target = 'windows' if IS_WINDOWS else 'linux'
    tools_dir = ROOT / ('build/tools-win' if IS_WINDOWS else 'build/tools-linux')
    build = ROOT / f'build/{target}-gow2'
    common = [cmake, '-S', ROOT, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', *flags]
    run([*common, '-B', tools_dir])
    run([cmake, '--build', tools_dir, '--target', 'eerecomp', 'vurecomp', '-j', args.jobs])
    compiler = tools_dir / ('eerecomp' + SUFFIX)
    vucompiler = tools_dir / ('vurecomp' + SUFFIX)
    entry_file = game / 'entries.txt'
    entry_file.touch()
    # Known addresses found while validating this exact executable are original
    # port configuration, not game code or data.
    known = ROOT / 'config/gow2-entries.txt'
    if known.is_file():
        entry_file.write_text('\n'.join(sorted(set(entry_file.read_text().split()) | set(known.read_text().split()))) + '\n')

    def translate():
        run([compiler, elf, '--entries', entry_file, '--out', generated])

    def compile_game():
        run([*common, '-B', build, f'-DPS2_GENERATED={generated}'])
        run([cmake, '--build', build, '--target', 'ps2run', '-j', args.jobs])
        if IS_WINDOWS:
            for dll in [*Path(cxx).parent.parent.glob('x86_64-w64-mingw32/bin/*.dll'),
                        sdl / 'bin/SDL2.dll', wgpu / 'lib/wgpu_native.dll']:
                shutil.copy2(dll, build / dll.name)

    translate()
    compile_game()
    runner = build / ('ps2run' + SUFFIX)
    report = {'platform': target, 'elf_sha256': sha, 'iso_bytes': iso.stat().st_size,
              'exploration_frames': args.frames, 'rounds': [], 'campaign_completed': False}
    if not args.no_explore:
        for index in range(args.rounds):
            misses = game / f'misses-{target}.txt'
            misses.unlink(missing_ok=True)
            log = game / f'explore-{target}.log'
            environment = dict(os.environ, PS2_VU_RECORD=str(game / 'vu1'), PS2_GPU_LOW_POWER='1')
            command = [runner, elf, '--disc', iso, '--ntsc', '--frames', args.frames,
                       '--misses', misses, '--memcard', game / f'explore-{target}.ps2',
                       '--frames-dir', game / 'shots', '--frame-every', 600, '--press', '300:start']
            for frame in range(600, args.frames, 300):
                command += ['--press', f'{frame}:cross']
            for frame in range(9800, args.frames, 30):
                command += ['--press', f'{frame}:square']
            if args.frames > 10000:
                command += ['--axis', '10000:lx:205:240']
            with log.open('w') as out:
                result = subprocess.run(list(map(str, command)), cwd=ROOT, env=environment, stdout=out, stderr=subprocess.STDOUT, timeout=1200)
            known_entries = set(entry_file.read_text().split())
            found = set(misses.read_text().split()) if misses.exists() else set()
            new = found - known_entries
            entry_file.write_text('\n'.join(sorted(known_entries | new)) + '\n')
            report['rounds'].append({'round': index + 1, 'exit_code': result.returncode, 'new_entries': len(new), 'log': str(log.relative_to(ROOT))})
            print(f'Rodada {index+1}: {len(new)} pontos novos; retorno {result.returncode}', flush=True)
            if not new:
                if result.returncode:
                    raise RuntimeError(f'O jogo parou. Consulte {log}')
                break
            translate()
            compile_game()
        else:
            raise RuntimeError('Limite de exploracao atingido; ainda ha codigo a descobrir.')
        run([vucompiler, game / 'vu1', generated])
        compile_game()
    report['built_at_utc'] = datetime.now(timezone.utc).isoformat()
    report['binary_sha256'] = hashlib.sha256(runner.read_bytes()).hexdigest()
    report['compiler'] = subprocess.check_output([cxx, '--version'], text=True).splitlines()[0]
    report['source_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    report['source_dirty'] = bool(subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=no'], cwd=ROOT, text=True).strip())
    (build / 'build-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    if args.package:
        run([sys.executable, ROOT / 'scripts/package_gow2.py', '--build', build])
    print(f'Pronto: {runner}\nAbra sem argumentos para usar a interface.', flush=True)


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        sys.exit(str(error))
