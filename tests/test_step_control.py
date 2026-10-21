#!/usr/bin/env python3
"""Integration test of deterministic headless control with a locally owned game."""
import argparse
import os
from pathlib import Path
import queue
import subprocess
import tempfile
import threading


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('runner','elf','iso','checkpoint'):
        p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='gow2-control-') as temporary:
        folder=Path(temporary)
        commands=[str(a.runner.resolve()),str(a.elf.resolve()),'--disc',str(a.iso.resolve()),'--ntsc',
                  '--interactive','--memcard',str(folder/'card.ps2')]
        log=(folder/'runtime.log').open('w')

        def start(checkpoint):
            child=subprocess.Popen([*commands,'--load-state',str(checkpoint.resolve())],stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,stderr=log,text=True,encoding='utf-8',bufsize=1)
            lines=queue.Queue()
            def reader():
                for line in child.stdout:
                    print(line,end='',flush=True);lines.put(line)
                lines.put(None)
            threading.Thread(target=reader,daemon=True).start()
            return child,lines

        def ready(lines):
            while True:
                line=lines.get(timeout=120)
                if line is None: raise RuntimeError('Runtime exited before READY')
                if '[control] ERROR' in line or 'CAPTURE FAIL' in line: raise RuntimeError(line)
                if line.startswith('[control] READY frame='): return int(line.split('=')[-1])

        def send(child,text):
            child.stdin.write(text+'\n');child.stdin.flush()

        child,lines=start(a.checkpoint)
        try:
            first=ready(lines)
            shot=folder/'capture.ppm'
            send(child,'capture '+str(shot));assert ready(lines)==first
            assert shot.read_bytes().startswith(b'P6\n') and shot.stat().st_size>1000
            send(child,'step 10 square lx=160');assert ready(lines)==first+10
            state=folder/'checkpoint.state'
            send(child,'save '+str(state));last=ready(lines);assert last==first+12
            assert state.is_file() and state.stat().st_size>1000000
            send(child,'quit');assert child.wait(timeout=30)==0
            child,lines=start(state)
            assert ready(lines)==last
            send(child,'step 3 -');assert ready(lines)==last+3
            send(child,'quit');assert child.wait(timeout=30)==0
            print('Step control, capture, checkpoint restart: PASS')
        finally:
            if child.poll() is None:
                child.kill();child.wait()
            log.close()


if __name__=='__main__':
    main()
