#!/usr/bin/env python3
"""Convert the Super Metroid soundtrack MP3s into PCM the XGM2 driver can stream.

The Mega Drive has no MP3 decoder: the game plays music as an 8-bit PCM sample on XGM2
channel 1, at 13.3 kHz mono. Each track is split into an intro, played once, and a loop
body, played with loop on. The rips play the loop twice and fade out, so only one loop
body is kept.

Both parts are cut to a multiple of 256 samples: XGM2 needs sample lengths in 256-byte
blocks and rescomp pads shorter ones with silence, which would be heard at every loop.

Sources stay local in res/sound/musics/ (not versioned); the WAVs written to
res/sound/music/ are. Needs ffmpeg (FFMPEG env var or PATH) and numpy.
Run from the project root: python scripts/convert_music.py [--find-loop TRACK]
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / 'res/sound/musics'
OUTPUT = ROOT / 'res/sound/music'
RATE = 13300  # XGM2 full rate.
BLOCK = 256
# Peak level of the music: the driver adds the 3 PCM channels, so the music leaves room for effects.
PEAK = 0.7
# name -> (source mp3, loop start, loop length), in samples at RATE. Found with --find-loop: the loop
# length is the lag where the track repeats (envelope autocorrelation, refined on the waveform), the
# start is where the repeat begins to match, rounded down to a block.
TRACKS = {
    # Landing Site (Crateria 1): 19.2 s of intro, then a 66.7 s loop.
    'music_crateria_arrival': ('03. Planet Zebes ~ Arrival on Crateria.mp3', 1000 * BLOCK, 3465 * BLOCK),
}


def ffmpeg() -> str:
    found = os.environ.get('FFMPEG') or shutil.which('ffmpeg')
    if not found:
        raise SystemExit('ffmpeg not found: install it or set FFMPEG to its path')
    return found


def decode(mp3: Path) -> np.ndarray:
    """Mono float samples at RATE."""
    raw = subprocess.run([ffmpeg(), '-v', 'error', '-i', str(mp3), '-ac', '1', '-ar', str(RATE),
                          '-f', 's16le', '-'], check=True, capture_output=True).stdout
    return np.frombuffer(raw, np.int16).astype(np.float64) / 32768


def write_wav(path: Path, samples: np.ndarray) -> None:
    data = np.clip(np.round(samples * 127 + 128), 0, 255).astype(np.uint8)
    with wave.open(str(path), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(1)
        out.setframerate(RATE)
        out.writeframes(data.tobytes())


def correlation(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.dot(a, b) / np.sqrt(np.dot(a, a) * np.dot(b, b) + 1e-12))


def find_loop(mp3: Path) -> None:
    """Print the loop length and how well each second matches one loop later."""
    x = decode(mp3)
    step = RATE // 100
    env = np.abs(x[:len(x) // step * step]).reshape(-1, step).mean(1)
    env -= env.mean()
    probe = env[2000:4000]  # 20 s from 20 s in
    lags = range(3000, min(13000, len(env) - 4000))
    lag = max(lags, key=lambda l: correlation(probe, env[2000 + l:4000 + l])) * step
    window, at = 3 * RATE, 30 * RATE
    lag = max(range(lag - 300, lag + 300),
              key=lambda l: correlation(x[at:at + window], x[at + l:at + l + window]))
    print(f'loop length {lag} samples ({lag / RATE:.3f} s), {lag // BLOCK} blocks')
    for second in range(0, 40):
        a = second * RATE
        print(f'{second:3d} s  match {correlation(x[a:a + RATE // 2], x[a + lag:a + lag + RATE // 2]):+.3f}')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--find-loop', metavar='MP3', help='analyse a track in res/sound/musics')
    args = parser.parse_args()
    if args.find_loop:
        find_loop(SOURCES / args.find_loop)
        return
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name, (source, start, length) in TRACKS.items():
        if start % BLOCK or length % BLOCK:
            raise SystemExit(f'{name}: start and length must be multiples of {BLOCK}')
        x = decode(SOURCES / source)
        if start + length > len(x):
            raise SystemExit(f'{name}: loop ends after the track')
        x *= PEAK / np.abs(x[:start + length]).max()
        write_wav(OUTPUT / f'{name}_intro.wav', x[:start])
        write_wav(OUTPUT / f'{name}_loop.wav', x[start:start + length])
        print(f'{name}: intro {start / RATE:.1f} s, loop {length / RATE:.1f} s, '
              f'{(start + length) // 1024} KB of PCM')


if __name__ == '__main__':
    sys.exit(main())
