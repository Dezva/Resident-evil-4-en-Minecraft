#!/usr/bin/env python3
"""Build the standalone RE4 diagnostic package without any game/mod binaries."""
import hashlib
from pathlib import Path
import zipfile


def main():
    root = Path(__file__).resolve().parents[1]
    destination = root / "dist"
    destination.mkdir(exist_ok=True)
    archive_path = destination / "re4bridge-re4-reader-0.1.0-windows.zip"
    files = [("START-RE4-READER.bat", "START-RE4-READER.bat"),
             ("tools/re4_reader.py", "tools/re4_reader.py"),
             ("docs/RE4-READER.md", "README.md")]
    with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for source, name in files:
            data = (root / source).read_bytes()
            if source.endswith(".bat"):
                data = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
            archive.writestr(name, data)
    digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    (destination / (archive_path.name + ".sha256")).write_text(
        f"{digest}  {archive_path.name}\n", encoding="utf-8")
    print(archive_path.name, archive_path.stat().st_size, "bytes", digest)


if __name__ == "__main__":
    main()
