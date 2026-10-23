"""Run the smoke ROM through BlastEm's GDB API, without desktop input.

Build with scripts/build.ps1 -SmokeTest first. Only the process started here is stopped.
"""
import argparse
import os
from pathlib import Path
import socket
import subprocess
import threading
import time


def secondary_monitor():
    """Read Windows monitor work areas; never change the user's display settings."""
    import ctypes as ct
    from ctypes import wintypes as wt

    class MonitorInfo(ct.Structure):
        _fields_ = [('size', wt.DWORD), ('monitor', wt.RECT), ('work', wt.RECT),
                    ('flags', wt.DWORD), ('device', wt.WCHAR * 32)]

    user32 = ct.windll.user32
    if hasattr(user32, 'SetProcessDpiAwarenessContext'):
        user32.SetProcessDpiAwarenessContext(ct.c_void_p(-4))
    user32.GetMonitorInfoW.argtypes = [wt.HANDLE, ct.POINTER(MonitorInfo)]
    monitors = []
    callback_type = ct.WINFUNCTYPE(wt.BOOL, wt.HANDLE, wt.HDC, ct.POINTER(wt.RECT), wt.LPARAM)

    @callback_type
    def collect(handle, dc, rect, data):
        info = MonitorInfo()
        info.size = ct.sizeof(info)
        if user32.GetMonitorInfoW(handle, ct.byref(info)) and not info.flags & 1:
            r = info.work
            monitors.append((info.device, (r.left, r.top, r.right, r.bottom)))
        return True

    user32.EnumDisplayMonitors(None, None, collect, 0)
    return monitors[0] if monitors else None


def show_test_window(process, monitor):
    """Place only our SDL window, then show it without activating or raising it."""
    import ctypes as ct
    from ctypes import wintypes as wt
    user32 = ct.windll.user32
    callback_type = ct.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    user32.GetWindow.restype = wt.HWND
    user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
    user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ct.POINTER(wt.DWORD)]
    user32.GetWindowRect.argtypes = [wt.HWND, ct.POINTER(wt.RECT)]
    user32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ct.c_int]
    user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ct.c_int, ct.c_int, ct.c_int, ct.c_int, wt.UINT]
    user32.ShowWindow.argtypes = [wt.HWND, ct.c_int]
    # SDL creates its window after GDB resumes the CPU, so discovery runs beside GDB.
    deadline = time.monotonic() + 20
    found = []

    @callback_type
    def collect(hwnd, data):
        pid = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ct.byref(pid))
        if pid.value != process.pid or user32.GetWindow(hwnd, 4): return True
        name = ct.create_unicode_buffer(128)
        user32.GetClassNameW(hwnd, name, len(name))
        if name.value.startswith('SDL'):
            found.append(hwnd)
            return False
        return True

    while process.poll() is None and time.monotonic() < deadline:
        user32.EnumWindows(collect, 0)
        if found:
            hwnd = found[0]
            rect = wt.RECT()
            user32.GetWindowRect(hwnd, ct.byref(rect))
            left, top, right, bottom = monitor[1]
            width, height = rect.right - rect.left, rect.bottom - rect.top
            scale = min(1, (right-left)/max(1,width), (bottom-top)/max(1,height))
            width, height = int(width*scale), int(height*scale)
            # SWP_NOZORDER | SWP_NOACTIVATE, followed by SW_SHOWNOACTIVATE.
            if user32.SetWindowPos(hwnd, None, left+(right-left-width)//2,
                    top+(bottom-top-height)//2, width, height, 0x14):
                user32.ShowWindow(hwnd, 4)
                print(f'BlastEm visible on {monitor[0]} (secondary), without activation.', flush=True)
            else:
                print('Could not position the test window; keeping it hidden.', flush=True)
            return
        time.sleep(0.1)
    print('No test window found; GDB validation continues without a visible window.', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--blastem', default='D:/emuladores/blastem/blastem-win64-1.0.0/blastem.exe')
    parser.add_argument('--timeout', type=float, default=120)
    parser.add_argument('--gdb', default='D:/apps/sgdk/bin/gdb.exe')
    parser.add_argument('--rom', default='tests/out/smoke.bin', help='ROM path relative to the project root')
    parser.add_argument('--symbols', default='out/symbol.txt', help='SGDK symbol table relative to the project root')
    parser.add_argument('--capture-scenes', action='store_true', help='Read back VDP video memory through GDB')
    visibility = parser.add_mutually_exclusive_group()
    visibility.add_argument('--hidden', dest='hidden', action='store_true', default=True, help='Headless execution (default; VDP rendering remains enabled for readback)')
    visibility.add_argument('--visible', dest='hidden', action='store_false', help='Show the test only on a secondary monitor, without activation')
    parser.add_argument('--realtime', action='store_true', help='Run at 60 fps with audio instead of unthrottled')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    symbols = {}
    for line in (root / args.symbols).read_text().splitlines():
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
    monitor = secondary_monitor() if not args.hidden and hasattr(subprocess, 'STARTUPINFO') else None
    if not args.hidden and not monitor:
        raise SystemExit('Visible test requires a secondary monitor; no emulator started.')
    startup = None
    if hasattr(subprocess, 'STARTUPINFO'):
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = subprocess.SW_HIDE
    env = os.environ.copy()
    if not args.realtime:
        # BlastEm syncs emulation to audio output; SDL's disk driver with no delay removes the 60 fps
        # cap for this process only. Emulated timing (VInts, CPU/DMA budgets) is unchanged.
        env.update(SDL_AUDIODRIVER='disk', SDL_DISKAUDIOFILE=os.devnull, SDL_DISKAUDIODELAY='0')
    process = subprocess.Popen([args.blastem, '-D', str(root / args.rom)],
        cwd=Path(args.blastem).parent, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        creationflags=flags, startupinfo=startup, env=env)
    if monitor:
        threading.Thread(target=show_test_window, args=(process, monitor), daemon=True).start()
    try:
        # Wait for the listener without consuming its single accepted GDB connection.
        time.sleep(1)
        commands = [args.gdb, '-nx', '-batch', '-ex', 'set confirm off',
            '-ex', 'set remote supported-packets-packet off',
            '-ex', 'set remotetimeout 10', '-ex', 'target remote 127.0.0.1:1234', '-ex', f'break *0x{finish:x}',
            '-ex', 'continue', '-ex', f'printf "MM_SMOKE_RESULT=%d\\n", *(unsigned short*)0x{result:x}',
            ]
        if 'mm_smoke_failure' in symbols:
            failure = int(symbols['mm_smoke_failure'], 16) & 0xFFFFFF
            commands += ['-ex', f'printf "MM_SMOKE_FAILURE=0x%x\\n", *(unsigned int*)0x{failure:x}']
        if 'mm_smoke_perf' in symbols:
            perf = int(symbols['mm_smoke_perf'], 16) & 0xFFFFFF
            for index, label in enumerate(('NORMAL_CPU', 'NORMAL_DMA', 'NORMAL_MISSED', 'SPIN_CPU', 'SPIN_DMA', 'SPIN_MISSED',
                                           'NORMAL_Y0', 'NORMAL_YMIN', 'SPIN_Y0', 'SPIN_YMIN')):
                commands += ['-ex', f'printf "{label}=%d\\n", *(unsigned short*)0x{perf+index*2:x}']
        if 'mm_smoke_memory' in symbols:
            memory = int(symbols['mm_smoke_memory'], 16) & 0xFFFFFF
            for index, label in enumerate(('VRAM_TILES_AVAILABLE', 'VRAM_TILES_ROOM_0', 'VRAM_TILES_ROOM_1', 'HEAP_FREE_START',
                                           'HEAP_FREE_TRIP_1', 'HEAP_FREE_TRIP_2', 'HEAP_FREE_TRIP_3', 'HEAP_LARGEST_BLOCK')):
                commands += ['-ex', f'printf "{label}=%d\\n", *(unsigned short*)0x{memory+index*2:x}']
        if args.capture_scenes:
            # Install capture commands before continuing to the final breakpoint.
            video = int(symbols['mm_smoke_video'], 16) & 0xFFFFFF
            header = int(symbols['mm_smoke_video_header'], 16) & 0xFFFFFF
            chunk = int(symbols['mm_smoke_video_chunk'], 16) & 0xFFFFFF
            ready = int(symbols['mm_smoke_video_ready'], 16) & 0xFFFFFF
            for name in ('scene-vram.bin', 'scene-headers.bin'):
                (root / 'tests/out' / name).unlink(missing_ok=True)
            script = root / 'tests/out/scene-capture.gdb'
            script.write_text(f'''break *0x{ready:x}
commands
silent
if *(unsigned short*)0x{chunk:x} == 0
append binary memory scene-headers.bin 0x{header:x} 0x{header+152:x}
end
append binary memory scene-vram.bin 0x{video:x} 0x{video+8192:x}
continue
end
''')
            index = commands.index('continue') - 1
            commands[index:index] = ['-ex', 'source scene-capture.gdb']
        commands += ['-ex', 'disconnect']
        started = time.monotonic()
        completed = subprocess.run(commands, capture_output=True, text=True, timeout=args.timeout,
                                   creationflags=flags, cwd=root/'tests/out')
        output = completed.stdout + completed.stderr
        (root / 'tests/out/smoke-gdb.log').write_text(output)
        if completed.returncode or 'MM_SMOKE_RESULT=1' not in output:
            raise SystemExit('Smoke test failed; see tests/out/smoke-gdb.log\n' + output)
        print(f'SMOKE PASS (BlastEm GDB API, {time.monotonic()-started:.1f}s, ' +
              ('realtime audio' if args.realtime else 'unthrottled, no audio') +
              (', headless)' if args.hidden else ', secondary monitor)'))
        metrics = [line for line in output.splitlines() if line.startswith(('NORMAL_', 'SPIN_', 'VRAM_', 'HEAP_'))]
        if any(int(line.split('=', 1)[1]) for line in metrics):
            for line in metrics: print(line)
        if args.capture_scenes:
            subprocess.run(['python', str(root/'scripts/check_room_video.py')], cwd=root, check=True)
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)


if __name__ == '__main__':
    main()
