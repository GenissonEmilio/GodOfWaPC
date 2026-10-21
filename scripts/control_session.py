#!/usr/bin/env python3
"""Relay a native headless test session through stdin/stdout on both platforms.

Pass the normal runner arguments after --runner <executable>. Commands are
documented in docs/TESTING.md. This controller never injects desktop input.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import threading

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runner',required=True)
p.add_argument('--record',type=Path,help='Append commands and runtime output to local files')
p.add_argument('arguments',nargs=argparse.REMAINDER)
a=p.parse_args()
record=log=None
if a.record:
    a.record.parent.mkdir(parents=True,exist_ok=True)
    record=a.record.open('a',encoding='utf-8',buffering=1)
    log=a.record.with_name(a.record.name+'.log').open('a',encoding='utf-8',buffering=1)
child=subprocess.Popen([a.runner,*a.arguments,'--interactive'],stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace',bufsize=1)
def relay():
    for line in child.stdout:
        if log: log.write(line)
        print(line,end='',flush=True)
thread=threading.Thread(target=relay,daemon=True);thread.start()
try:
    for line in sys.stdin:
        if child.poll() is not None: break
        if record: record.write(line)
        child.stdin.write(line);child.stdin.flush()
        if line.strip()=='quit': break
    if child.poll() is None: child.stdin.close()
    result=child.wait()
    thread.join(timeout=5)
    raise SystemExit(result)
except (KeyboardInterrupt,BrokenPipeError):
    if child.poll() is None:
        try:
            child.stdin.write('quit\n');child.stdin.flush();child.wait(timeout=30)
        except (BrokenPipeError,subprocess.TimeoutExpired):
            child.kill();child.wait()
    raise SystemExit(130)
finally:
    if record: record.close()
    if log: log.close()
