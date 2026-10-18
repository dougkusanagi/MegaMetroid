"""Run the smoke ROM through BlastEm's GDB API, without desktop input.

Build with scripts/build.ps1 -SmokeTest first. Only the process started here is stopped.
"""
import argparse
from pathlib import Path
import socket
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--blastem', default='D:/emuladores/blastem/blastem-win64-1.0.0/blastem.exe')
    parser.add_argument('--timeout', type=float, default=45)
    parser.add_argument('--gdb', default='D:/apps/sgdk/bin/gdb.exe')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    symbols = {}
    for line in (root / 'out/symbol.txt').read_text().splitlines():
        parts = line.split()
        if len(parts) >= 3:
            symbols[parts[2]] = parts[0]
    finish = int(symbols['mm_smoke_finish'], 16) & 0xFFFFFF
    result = int(symbols['mm_smoke_result'], 16) & 0xFFFFFF
    # On Windows -D serves GDB on localhost:1234. Never attach to an unrelated emulator.
    with socket.socket() as probe:
        if probe.connect_ex(('127.0.0.1', 1234)) == 0:
            raise SystemExit('Port 1234 already in use; close that debug session before running this test.')
    flags = subprocess.CREATE_NO_WINDOW if hasattr(subprocess, 'CREATE_NO_WINDOW') else 0
    startup = None
    if hasattr(subprocess, 'STARTUPINFO'):
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = subprocess.SW_HIDE
    process = subprocess.Popen([args.blastem, '-D', str(root / 'tests/out/smoke.bin')],
        cwd=Path(args.blastem).parent, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        creationflags=flags, startupinfo=startup)
    try:
        # Wait for the listener without consuming its single accepted GDB connection.
        time.sleep(1)
        commands = [args.gdb, '-nx', '-batch', '-ex', 'set confirm off',
            '-ex', 'target remote localhost:1234', '-ex', f'break *0x{finish:x}',
            '-ex', 'continue', '-ex', f'printf "MM_SMOKE_RESULT=%d\\n", *(unsigned short*)0x{result:x}',
            ]
        if 'mm_smoke_failure' in symbols:
            failure = int(symbols['mm_smoke_failure'], 16) & 0xFFFFFF
            commands += ['-ex', f'printf "MM_SMOKE_FAILURE=0x%x\\n", *(unsigned int*)0x{failure:x}']
        commands += ['-ex', 'disconnect']
        completed = subprocess.run(commands, capture_output=True, text=True, timeout=args.timeout,
                                   creationflags=flags)
        output = completed.stdout + completed.stderr
        (root / 'tests/out/smoke-gdb.log').write_text(output)
        if completed.returncode or 'MM_SMOKE_RESULT=1' not in output:
            raise SystemExit('Smoke test failed; see tests/out/smoke-gdb.log\n' + output)
        print('SMOKE PASS (BlastEm GDB API, no desktop input)')
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)


if __name__ == '__main__':
    main()
