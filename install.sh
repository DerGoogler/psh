#!/data/data/com.termux/files/usr/bin/bash

set -e

BINARY_NAME="psh"
VERSION="${VERSION:-latest}"

# Detect if running as root inside Termux
if [ "$(id -u)" -eq 0 ]; then
    echo "Error: Do not run this installer as root inside Termux."
    exit 1
fi

ARCH=$(uname -m)

case "$ARCH" in
    aarch64)
        ARCH_DL="arm64-v8a"
        ;;
    armv7l|armv8l)
        ARCH_DL="armeabi-v7a"
        ;;
    x86_64)
        ARCH_DL="x86_64"
        ;;
    i686)
        ARCH_DL="x86"
        ;;
    *)
        echo "Error: Unsupported architecture: $ARCH"
        exit 1
        ;;
esac

ONLINE_BINARY="${BINARY_NAME}-${ARCH_DL}"

if [ "$VERSION" = "latest" ]; then
    DOWNLOAD_URL="https://github.com/DerGoogler/psh/releases/latest/download/$ONLINE_BINARY"
else
    DOWNLOAD_URL="https://github.com/DerGoogler/psh/releases/download/${VERSION}/$ONLINE_BINARY"
fi

DEST_DIR="$PREFIX/usr/bin"
DEST_PATH="${DEST_DIR}/${BINARY_NAME}"

echo "Installing ${BINARY_NAME} version ${VERSION} for architecture ${ARCH_DL}..."
echo "Downloading from: ${DOWNLOAD_URL}"

curl -L --fail "$DOWNLOAD_URL" -o "$DEST_PATH"

chmod 755 "$DEST_PATH"

TERMUX_UID=$(stat -c "%u" "$PREFIX")
TERMUX_GID=$(stat -c "%g" "$PREFIX")
chown "$TERMUX_UID:$TERMUX_GID" "$DEST_PATH"

echo "Installation successful!"
echo "Binary installed at: $DEST_PATH"
