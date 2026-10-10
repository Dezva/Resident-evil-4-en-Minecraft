#!/usr/bin/env bash
set -euo pipefail
native_root=/workspace/tools/mingw32
if [ ! -x "$native_root/usr/bin/i686-w64-mingw32-gcc-posix" ]; then
  mkdir -p /workspace/tools/mingw-downloads /workspace/.cache/apt/lists/partial /workspace/.cache/apt/archives/partial
  native_tmp=$(mktemp -d /tmp/re4-native-apt.XXXXXX)
  trap 'rm -rf "$native_tmp"' EXIT
  mkdir "$native_tmp/parts"
  cat > "$native_tmp/config" <<EOF
Dir::Etc::parts "$native_tmp/parts";
Dir::State::lists "/workspace/.cache/apt/lists";
Dir::Cache::archives "/workspace/.cache/apt/archives";
APT::Sandbox::User "agent";
EOF
  APT_CONFIG="$native_tmp/config" apt-get update
  cd /workspace/tools/mingw-downloads
  APT_CONFIG="$native_tmp/config" apt-get download \
    gcc-mingw-w64-i686-posix=14.2.0-19+27+b1 \
    gcc-mingw-w64-i686-posix-runtime=14.2.0-19+27+b1 \
    gcc-mingw-w64-base=14.2.0-19+27+b1 \
    mingw-w64-i686-dev=12.0.0-5 mingw-w64-common=12.0.0-5 binutils-mingw-w64-i686=2.44-3+12+b1
  mkdir -p "$native_root"
  for package in ./*.deb; do dpkg-deb -x "$package" "$native_root"; done
fi
"$native_root/usr/bin/i686-w64-mingw32-gcc-posix" --version
