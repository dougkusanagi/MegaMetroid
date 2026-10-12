"""Finalize a conventional Genesis cartridge header, then verify the artifact."""
from pathlib import Path
import hashlib
import sys

rom = Path(sys.argv[1])
data = bytearray(rom.read_bytes())
assert len(data) >= 512 and len(data) % 2 == 0
assert data[0x100:0x110] == b'SEGA MEGA DRIVE '
data[0x1a4:0x1a8] = (len(data) - 1).to_bytes(4, 'big')
checksum = sum(int.from_bytes(data[i:i+2], 'big') for i in range(512, len(data), 2)) & 0xffff
data[0x18e:0x190] = checksum.to_bytes(2, 'big')
rom.write_bytes(data)
assert int.from_bytes(data[0x1a4:0x1a8], 'big') + 1 == len(data)
print(f'ROM: {len(data)} bytes, Genesis checksum {checksum:04X}, SHA256 {hashlib.sha256(data).hexdigest()}')
