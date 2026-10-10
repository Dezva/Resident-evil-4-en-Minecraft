#!/usr/bin/env python3
"""Package the diagnostic client and simulator; never include game files or local configs."""
import argparse
import hashlib
import json
from pathlib import Path
import urllib.request
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fabric-api", required=True, type=Path, help="Original Fabric API 0.116.6+1.21.1 JAR")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    version = "0.2.0"
    jar = root / f"minecraft/build/libs/re4bridge-{version}.jar"
    api = args.fabric_api
    expected = urllib.request.urlopen(
        "https://maven.fabricmc.net/net/fabricmc/fabric-api/fabric-api/0.116.6+1.21.1/"
        "fabric-api-0.116.6+1.21.1.jar.sha1", timeout=30).read().decode().strip().split()[0]
    if hashlib.sha1(api.read_bytes()).hexdigest() != expected:
        raise SystemExit("Fabric API checksum does not match the official Maven artifact")
    with zipfile.ZipFile(jar) as mod:
        metadata = json.loads(mod.read("fabric.mod.json"))
        if metadata["version"] != version or metadata["depends"]["minecraft"] != "1.21.1":
            raise SystemExit("Wrong mod version")
        with mod.open(metadata["jars"][0]["file"]) as nested:
            with zipfile.ZipFile(nested) as core:
                if "dev/dezva/re4bridge/BridgeConnection.class" not in core.namelist():
                    raise SystemExit("Missing bridge core")
    destination = root / "dist"
    destination.mkdir(exist_ok=True)
    package = destination / f"re4bridge-{version}-windows.zip"
    with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED) as archive:
        for file, name in [(jar, "mods/" + jar.name), (api, "mods/" + api.name),
                           (root / "START-SIMULATOR.bat", "START-SIMULATOR.bat"),
                           (root / "tools/fake_re4_host.py", "tools/fake_re4_host.py"),
                           (root / "START-RE4-BRIDGE.bat", "START-RE4-BRIDGE.bat"),
                           (root / "tools/re4_live_host.py", "tools/re4_live_host.py"),
                           (root / "tools/re4_reader.py", "tools/re4_reader.py"),
                           (root / "docs/RE4-LIVE.md", "README.md"),
                           (root / "docs/VALIDATION.md", "docs/VALIDATION.md"),
                           (root / "docs/CONTRACT.md", "docs/CONTRACT.md")]:
            data = file.read_bytes()
            if file.suffix == ".bat":
                data = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
            archive.writestr(name, data)
    # Source archive excludes every runtime/build/cache tree and any generated token.
    source = destination / f"re4bridge-{version}-source.zip"
    excluded = {"__pycache__", "re4bridge.json"}
    top_level = [".gitignore", "README.md", "MODLOG.md", "build.gradle", "settings.gradle",
                 "gradle.properties", "gradlew", "gradlew.bat", "START-SIMULATOR.bat", "START-RE4-BRIDGE.bat", "START-RE4-READER.bat",
                 "bridge-core/build.gradle", "minecraft/build.gradle"]
    files = [root / name for name in top_level]
    for directory in ["bridge-core/src", "minecraft/src", "tools", "docs", "gradle"]:
        files.extend((root / directory).rglob("*"))
    with zipfile.ZipFile(source, "w", zipfile.ZIP_DEFLATED) as archive:
        for file in sorted(set(files)):
            rel = file.relative_to(root)
            if file.is_file() and not excluded.intersection(rel.parts) and file.suffix != '.pyc' and not file.name.endswith('.env'):
                archive.write(file, rel.as_posix())
    sums = []
    for path in (package, source):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        sums.append(f"{digest}  {path.name}")
        print(path.name, path.stat().st_size, "bytes")
    (destination / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
