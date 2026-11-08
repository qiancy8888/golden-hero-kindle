#!/bin/sh
# Install KOReader's prebuilt Kindle 5 (Kindle 4/Touch/PW1) cross toolchain.
set -eu
TAG=2026.08
URL="https://github.com/koreader/koxtoolchain/releases/download/${TAG}/kindle5.tar.zst"
SHA256=4f519d30c6b041ba3e8a34f2b62437232b8b27e672c1b892c8d350c0f7706c98
DEST="${HOME}/x-tools/arm-kindle5-linux-gnueabi"

if [ -x "$DEST/bin/arm-kindle5-linux-gnueabi-g++" ]; then
  echo "Kindle5 toolchain already installed: $DEST"
  exit 0
fi
for cmd in curl sha256sum tar; do
  command -v "$cmd" >/dev/null 2>&1 || { echo "Missing required command: $cmd" >&2; exit 2; }
done
if ! tar --help 2>&1 | grep -q -- '--zstd'; then
  echo "GNU tar with zstd support is required (install zstd)." >&2
  exit 2
fi
mkdir -p "$HOME"
TMP=$(mktemp "${TMPDIR:-/tmp}/kindle5-toolchain.XXXXXX.tar.zst")
cleanup() { rm -f "$TMP"; }
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

echo "Downloading Kindle5 toolchain (~95 MB compressed)..."
curl -fL --retry 3 --connect-timeout 20 "$URL" -o "$TMP"
echo "$SHA256  $TMP" | sha256sum -c -
echo "Extracting into $HOME (about 420 MB unpacked)..."
tar --zstd -xf "$TMP" -C "$HOME"
if [ ! -x "$DEST/bin/arm-kindle5-linux-gnueabi-g++" ]; then
  echo "Toolchain extraction did not create expected compiler: $DEST/bin" >&2
  exit 3
fi
echo "Installed: $DEST"
echo "Compiler: $DEST/bin/arm-kindle5-linux-gnueabi-g++"
