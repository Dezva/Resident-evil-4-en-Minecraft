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
    version = "0.4.33"
    mod_version = "0.4.33"
    native_hash = hashlib.sha256((root / "native/build/dinput8.dll").read_bytes()).hexdigest()
    if native_hash != "f707036409bad6f7044baf49fdc29f790af8f654ff11617730aa350055968a11":
        raise SystemExit("0.4.33 requires the verified build433 GUI/hand/selection native DLL")
    jar = root / f"minecraft/build/libs/re4bridge-{mod_version}.jar"
    api = args.fabric_api
    expected = urllib.request.urlopen(
        "https://maven.fabricmc.net/net/fabricmc/fabric-api/fabric-api/0.116.6+1.21.1/"
        "fabric-api-0.116.6+1.21.1.jar.sha1", timeout=30).read().decode().strip().split()[0]
    if hashlib.sha1(api.read_bytes()).hexdigest() != expected:
        raise SystemExit("Fabric API checksum does not match the official Maven artifact")
    with zipfile.ZipFile(jar) as mod:
        metadata = json.loads(mod.read("fabric.mod.json"))
        if metadata["version"] != mod_version or metadata["depends"]["minecraft"] != "1.21.1":
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
                           (root / "START-RE4-CONTROLS.bat", "START-RE4-CONTROLS.bat"),
                           (root / "VIEW-RE4.bat", "VIEW-RE4.bat"),
                           (root / "VIEW-RE4.ps1", "VIEW-RE4.ps1"),
                           (root / "tools/RE4View.cs", "tools/RE4View.cs"),
                           (root / "docs/RE4-VIEW.md", "docs/RE4-VIEW.md"),
                           (root / "DIAGNOSE-RE4-CONTROLS.bat", "DIAGNOSE-RE4-CONTROLS.bat"),
                           (root / "DIAGNOSE-RE4-CONTROLS.ps1", "DIAGNOSE-RE4-CONTROLS.ps1"),
                           (root / "PROBE-RE4-NATIVE.bat", "PROBE-RE4-NATIVE.bat"),
                           (root / "tools/re4_native_probe.py", "tools/re4_native_probe.py"),
                           (root / "PROBE-RE4-GRAPHICS.bat", "PROBE-RE4-GRAPHICS.bat"),
                           (root / "tools/re4_graphics_probe.py", "tools/re4_graphics_probe.py"),
                           (root / "CAPTURE-RE4-GRAPHICS.bat", "CAPTURE-RE4-GRAPHICS.bat"),
                           (root / "tools/re4_graphics_capture.py", "tools/re4_graphics_capture.py"),
                           (root / "TEST-RE4-CUBE.bat", "TEST-RE4-CUBE.bat"),
                           (root / "tools/re4_cube_test.py", "tools/re4_cube_test.py"),
                           (root / "AUDIT-RE4-IMPORTS.bat", "AUDIT-RE4-IMPORTS.bat"),
                           (root / "tools/re4_import_audit.py", "tools/re4_import_audit.py"),
                           (root / "docs/RE4-IMPORT-AUDIT.md", "docs/RE4-IMPORT-AUDIT.md"),
                           (root / "docs/RE4-GRAPHICS-PROBE.md", "docs/RE4-GRAPHICS-PROBE.md"),
                           (root / "INSTALL-RE4-CONTROLS.ps1", "INSTALL-RE4-CONTROLS.ps1"),
                           (root / "UNINSTALL-RE4-CONTROLS.ps1", "UNINSTALL-RE4-CONTROLS.ps1"),
                           (root / "INSTALL-RE4-CONTROLS.bat", "INSTALL-RE4-CONTROLS.bat"),
                           (root / "UNINSTALL-RE4-CONTROLS.bat", "UNINSTALL-RE4-CONTROLS.bat"),
                           (root / "native/build/dinput8.dll", "native/dinput8.dll"),
                           (root / "tools/re4_live_host.py", "tools/re4_live_host.py"),
                           (root / "tools/re4_control.py", "tools/re4_control.py"),
                           (root / "tools/re4_hud.py", "tools/re4_hud.py"),
                           (root / "tools/re4_overlay.py", "tools/re4_overlay.py"),
                           (root / "tools/re4_block_preview.py", "tools/re4_block_preview.py"),
                           (root / "tools/re4_reader.py", "tools/re4_reader.py"),
                           (root / "TRACE-RE4-CAMERA.bat", "TRACE-RE4-CAMERA.bat"),
                           (root / "MEASURE-RE4-MOVEMENT.bat", "MEASURE-RE4-MOVEMENT.bat"),
                           (root / "EXPORT-RE4-MAP.bat", "EXPORT-RE4-MAP.bat"),
                           (root / "tools/re4_map_export.py", "tools/re4_map_export.py"),
                           (root / "tools/re4_map_viewer.html", "tools/re4_map_viewer.html"),
                           (root / "tools/re4_movement_measure.py", "tools/re4_movement_measure.py"),
                           (root / "tools/re4_camera_trace.py", "tools/re4_camera_trace.py"),
                           (root / "docs/RE4-CAMERA-TRACE.md", "README-TRACE.md"),
                           (root / "docs/RE4-BLOCK-0.4.33.md", "README.md"),
                           (root / "docs/RE4-RECOVERY-0.4.14.md", "docs/RE4-RECOVERY-0.4.14.md"),
                           (root / "COLLECT-RE4-CRASH.bat", "COLLECT-RE4-CRASH.bat"),
                           (root / "COLLECT-RE4-CRASH.ps1", "COLLECT-RE4-CRASH.ps1"),
                           (root / "docs/RE4-CONTROLS.md", "docs/RE4-CONTROLS.md"),
                           (root / "docs/RE4-LIVE.md", "docs/RE4-LIVE.md"),
                           (root / "docs/VALIDATION.md", "docs/VALIDATION.md"),
                           (root / "docs/CONTRACT.md", "docs/CONTRACT.md")]:
            data = file.read_bytes()
            if file.suffix == ".bat":
                data = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
            archive.writestr(name, data)
        archive.writestr("NATIVE-SHA256.txt", native_hash + "  native/dinput8.dll\n")
    # Source archive excludes every runtime/build/cache tree and any generated token.
    source = destination / f"re4bridge-{version}-source.zip"
    excluded = {"__pycache__", "re4bridge.json", "build"}
    top_level = [".gitignore", "README.md", "MODLOG.md", "build.gradle", "settings.gradle",
                 "gradle.properties", "gradlew", "gradlew.bat", "START-SIMULATOR.bat", "START-RE4-BRIDGE.bat", "START-RE4-READER.bat",
                 "START-RE4-CONTROLS.bat", "INSTALL-RE4-CONTROLS.ps1", "UNINSTALL-RE4-CONTROLS.ps1",
                 "DIAGNOSE-RE4-CONTROLS.bat", "DIAGNOSE-RE4-CONTROLS.ps1",
                 "PROBE-RE4-NATIVE.bat", "PROBE-RE4-GRAPHICS.bat", "CAPTURE-RE4-GRAPHICS.bat", "TEST-RE4-CUBE.bat", "AUDIT-RE4-IMPORTS.bat", "COLLECT-RE4-CRASH.bat", "COLLECT-RE4-CRASH.ps1", "VIEW-RE4.bat", "VIEW-RE4.ps1", "TRACE-RE4-CAMERA.bat", "MEASURE-RE4-MOVEMENT.bat", "EXPORT-RE4-MAP.bat",
                 "INSTALL-RE4-CONTROLS.bat", "UNINSTALL-RE4-CONTROLS.bat",
                 "bridge-core/build.gradle", "minecraft/build.gradle"]
    files = [root / name for name in top_level]
    for directory in ["bridge-core/src", "minecraft/src", "tools", "docs", "gradle", "native"]:
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
