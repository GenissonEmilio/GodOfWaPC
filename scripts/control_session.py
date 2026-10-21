#!/usr/bin/env python3
"""Relay a native headless test session through stdin/stdout on both platforms.

Pass the normal runner arguments after --runner <executable>. Commands are
documented in docs/TESTING.md. This controller never injects desktop input.
"""
import argparse
import subprocess
import sys
import threading

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runner',required=True)
p.add_argument('arguments',nargs=argparse.REMAINDER)
a=p.parse_args()
child=subprocess.Popen([a.runner,*a.arguments,'--interactive'],stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace',bufsize=1)
def relay():
    for line in child.stdout:
        print(line,end='',flush=True)
thread=threading.Thread(target=relay,daemon=True);thread.start()
try:
    for line in sys.stdin:
        if child.poll() is not None: break
        child.stdin.write(line);child.stdin.flush()
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
