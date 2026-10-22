#!/usr/bin/env python3
"""Replay a locally recorded gameplay case and compare real renderer captures.

Scenarios, reference images, cards and checkpoints stay local. A passing case
proves the specified sequence, not completion of a campaign or an altar save.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import queue
import re
import shutil
import subprocess
import threading


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ppm(path):
    magic, size, depth, pixels = path.read_bytes().split(b'\n', 3)
    width, height = map(int, size.split())
    if magic != b'P6' or depth != b'255' or len(pixels) != width * height * 3:
        raise ValueError(f'Unsupported renderer capture: {path}')
    return (width, height), pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runner', 'elf', 'iso', 'scenario', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--timeout', type=float, default=120, help='Seconds per step or capture')
    args = parser.parse_args()
    runner, elf, iso, scenario_path = (getattr(args, name).resolve(strict=True)
                                      for name in ('runner', 'elf', 'iso', 'scenario'))
    scenario = json.loads(scenario_path.read_text(encoding='utf-8'))
    actions = scenario['actions']
    if not actions or not any('capture' in action and 'reference' in action for action in actions):
        parser.error('A scenario needs at least one capture with a reviewed reference image.')
    output = args.output.resolve()
    if any('\n' in str(path) or '\r' in str(path) for path in (output,)):
        parser.error('Output path cannot contain a newline.')
    output.mkdir(parents=True, exist_ok=False)

    def local(name):
        return (scenario_path.parent / scenario[name]).resolve(strict=True)

    card = output / 'card.ps2'
    initial_card = local('card') if scenario.get('card') else None
    if initial_card:
        shutil.copy2(initial_card, card)
    command = [str(runner), str(elf), '--disc', str(iso), '--ntsc', '--interactive', '--memcard', str(card)]
    checkpoint = local('checkpoint') if scenario.get('checkpoint') else None
    if checkpoint:
        command += ['--load-state', str(checkpoint)]
    report = {'name': scenario['name'], 'passed': False,
              'started_at_utc': datetime.now(timezone.utc).isoformat(),
              'binary_sha256': digest(runner), 'elf_sha256': digest(elf),
              'scenario_sha256': digest(scenario_path),
              'checkpoint_sha256': digest(checkpoint) if checkpoint else None,
              'initial_card_sha256': digest(initial_card) if initial_card else None, 'observations': []}
    messages = queue.Queue()
    child = None
    reader = None
    buttons = {'select', 'l3', 'r3', 'start', 'up', 'right', 'down', 'left', 'l2', 'r2',
               'l1', 'r1', 'triangle', 'circle', 'cross', 'square', 'lup', 'ldown', 'lleft', 'lright'}
    with (output / 'runtime.log').open('w', encoding='utf-8', buffering=1) as log:
        try:
            child = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, text=True, encoding='utf-8', errors='replace', bufsize=1)

            def read_output():
                for line in child.stdout:
                    log.write(line)
                    if line.startswith('[control]'):
                        messages.put(line)
                messages.put(None)

            reader = threading.Thread(target=read_output, daemon=True)
            reader.start()

            def ready():
                while True:
                    line = messages.get(timeout=args.timeout)
                    if line is None:
                        raise RuntimeError('Runtime exited before READY; inspect runtime.log')
                    if '[control] ERROR' in line or 'CAPTURE FAIL' in line:
                        raise RuntimeError(line.strip())
                    match = re.fullmatch(r'\[control\] READY frame=(\d+)\s*', line)
                    if match:
                        return int(match[1])

            def send(text):
                child.stdin.write(text + '\n')
                child.stdin.flush()

            frame = ready()
            report['initial_frame'] = frame
            for index, action in enumerate(actions):
                if 'frames' in action:
                    count = action['frames']
                    held = action.get('buttons', '-')
                    axes = action.get('axes', {})
                    if type(count) is not int or not 1 <= count <= 1000000:
                        raise ValueError('Step duration must be 1..1000000 frames')
                    if held != '-' and any(name not in buttons for name in held.split('+')):
                        raise ValueError('Unknown button name')
                    if any(axis not in {'lx', 'ly', 'rx', 'ry'} or type(value) is not int or not 0 <= value <= 255
                           for axis, value in axes.items()):
                        raise ValueError('Invalid analog axis')
                    send(f'step {count} {held}' + ''.join(f' {axis}={value}' for axis, value in axes.items()))
                    expected = frame + count
                    frame = ready()
                    if frame != expected:
                        raise RuntimeError(f'Expected frame {expected}, got {frame}')
                elif 'capture' in action:
                    capture = output / f'capture-{index:03}.ppm'
                    reference = (scenario_path.parent / action['reference']).resolve(strict=True)
                    send('capture ' + str(capture))
                    if ready() != frame:
                        raise RuntimeError('Capture unexpectedly advanced the game')
                    shape, pixels = ppm(capture)
                    expected_shape, expected_pixels = ppm(reference)
                    if shape != expected_shape:
                        raise RuntimeError('Capture dimensions differ from the reference')
                    error = sum(abs(a - b) for a, b in zip(pixels, expected_pixels)) / len(pixels)
                    limit = float(action.get('max_mean_error', 1.0))
                    if not 0 <= limit <= 255:
                        raise ValueError('Invalid image comparison limit')
                    report['observations'].append({'capture': action['capture'], 'frame': frame,
                        'path': capture.name, 'sha256': digest(capture), 'reference_sha256': digest(reference),
                        'mean_rgb_error': error, 'max_mean_error': limit})
                    if error > limit:
                        raise RuntimeError(f'Capture {action["capture"]}: mean error {error:.6f} exceeds {limit}')
                    print(f'{action["capture"]}: frame {frame}, mean RGB error {error:.6f}', flush=True)
                else:
                    raise ValueError('Each action must be a frame step or a reference capture')
            send('quit')
            report['exit_code'] = child.wait(timeout=30)
            if report['exit_code']:
                raise RuntimeError(f'Runtime exited with {report["exit_code"]}')
            report['final_frame'] = frame
            report['card_written'] = card.is_file()
            report['card_sha256'] = digest(card) if card.is_file() else None
            report['passed'] = True
        except Exception as error:
            report['error'] = str(error)
            raise
        finally:
            if child is not None and child.poll() is None:
                child.kill()
                child.wait()
            if reader:
                reader.join(timeout=5)
            (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'{scenario["name"]}: PASS', flush=True)


if __name__ == '__main__':
    main()
