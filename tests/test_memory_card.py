#!/usr/bin/env python3
"""Check real SIO2 card persistence using separate writer/reader processes.

These tests verify the virtual card protocol and host writes, not a campaign
save made at an altar inside God of War II.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', required=True, type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    runner = args.runner.resolve(strict=True)
    results = []
    with tempfile.TemporaryDirectory(prefix='gow2-card-') as temporary:
        root = Path(temporary)

        def run(mode, card, variant):
            result = subprocess.run([str(runner), mode, str(card), str(variant)],
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            print(result.stdout, end='', flush=True)
            results.append({'mode': mode, 'variant': variant, 'exit_code': result.returncode})
            if result.returncode:
                raise RuntimeError('Memory card verification failed')

        card = root / 'restart.ps2'
        run('write', card, 1)
        run('read', card, 1)
        first = hashlib.sha256(card.read_bytes()).hexdigest()
        run('write', card, 2)
        run('read', card, 2)
        assert hashlib.sha256(card.read_bytes()).hexdigest() != first
        run('force', card, 3)
        run('read', card, 3)
        run('blocked', card, 5)
        run('read', card, 5)
        retry = root / 'initially-missing' / 'retry.ps2'
        run('retry', retry, 4)
        run('read', retry, 4)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps({'platform': os.name, 'scope': 'SIO2 protocol and file persistence',
                                         'in_game_altar_save_tested': False, 'tests': results}, indent=2)+'\n')


if __name__ == '__main__':
    main()
