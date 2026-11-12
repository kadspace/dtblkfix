"""Packs files into the resource that the single-file plugin carries (unpacked by
plugin/EngineBridge.cpp).

  pack_resources.py <out> <name>=<file or folder> ...

A folder is packed with everything in it (except .done), under <name>. A later name replaces an
earlier one. The format: "DTBXPAK1", then for each file a little-endian u32 name length, the name
(UTF-8, with backslashes), a u32 size and the data; a name length of 0 ends it.
"""

import os
import struct
import sys


def collect(specs):
    files = {}
    for spec in specs:
        name, src = spec.split("=", 1)
        if os.path.isdir(src):
            for root, dirs, names in os.walk(src):
                dirs.sort()
                for n in sorted(names):
                    if n == ".done":
                        continue
                    path = os.path.join(root, n)
                    files[os.path.join(name, os.path.relpath(path, src))] = path
        elif os.path.isfile(src):
            files[name] = src
        else:
            sys.exit(f"pack_resources.py: {src} doesn't exist")
    return {k.replace("/", "\\"): v for k, v in files.items()}


def main(out, specs):
    files = collect(specs)
    with open(out + ".tmp", "wb") as f:
        f.write(b"DTBXPAK1")
        for name in sorted(files):
            if ".." in name.split("\\"):
                sys.exit(f"pack_resources.py: bad name {name}")
            with open(files[name], "rb") as src:
                data = src.read()
            encoded = name.encode("utf-8")
            f.write(struct.pack("<I", len(encoded)) + encoded + struct.pack("<I", len(data)) + data)
        f.write(struct.pack("<I", 0))
    os.replace(out + ".tmp", out)
    print(f"packed {len(files)} files into {out}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2:])
