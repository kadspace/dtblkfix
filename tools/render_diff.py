import struct
import sys

import numpy as np


def load(path):
    out = []
    with open(path, "rb") as f:
        while True:
            name = f.read(32)
            if len(name) < 32:
                break
            sync, n = struct.unpack("<ii", f.read(8))
            l = np.frombuffer(f.read(4 * n), dtype="<f4")
            r = np.frombuffer(f.read(4 * n), dtype="<f4")
            out.append((name.split(b"\0")[0].decode("latin-1"), sync, np.concatenate([l, r])))
    return out


def main():
    a, b = load(sys.argv[1]), load(sys.argv[2])
    worst = -999.0
    identical = 0
    print(f"{'#':>2} {'preset':24} sync  identical  max|diff|   diff vs signal")
    for i, ((na, sa, xa), (nb, sb, xb)) in enumerate(zip(a, b)):
        d = xa.astype(np.float64) - xb.astype(np.float64)
        same = np.array_equal(xa.view(np.uint32), xb.view(np.uint32))
        identical += same
        sig = np.sqrt(np.mean(xa.astype(np.float64) ** 2)) + 1e-30
        err = np.sqrt(np.mean(d**2))
        rel = 20 * np.log10(err / sig) if err > 0 else -np.inf
        worst = max(worst, rel)
        print(f"{i:2d} {na:24} {sa:4d}  {'yes' if same else 'no':9}  {np.max(np.abs(d)):.3e}   {rel:7.1f} dB")
    print(f"{identical}/{len(a)} presets bit-identical; worst difference {worst:.1f} dB relative to the signal")


main()
