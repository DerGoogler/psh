#!/bin/bash

# --- Configuration ---
NDK_ROOT="/home/jimmy/android-ndk-r27c"
PROJECT_ROOT="$(pwd)"
TERMUX_BIN="/data/data/com.termux/files/usr/bin"
BINARY_NAME="psh"

# Check for --release flag
RELEASE_MODE=false
if [[ "$1" == "--release" ]]; then
    RELEASE_MODE=true
fi

echo "Starting NDK build for $BINARY_NAME..."
echo "NDK Root: $NDK_ROOT"
echo "Project Root: $PROJECT_ROOT"

if [ ! -d "$NDK_ROOT" ]; then
    echo "Error: NDK_ROOT directory '$NDK_ROOT' not found."
    exit 1
fi

if [ ! -f "$PROJECT_ROOT/Android.mk" ] || [ ! -f "$PROJECT_ROOT/Application.mk" ]; then
    echo "Error: Missing one or more required project files."
    exit 1
fi

"$NDK_ROOT/ndk-build" NDK_PROJECT_PATH="$PROJECT_ROOT" APP_BUILD_SCRIPT="$PROJECT_ROOT/Android.mk"

if [ $? -eq 0 ]; then
    echo "NDK build successful!"
    
    if $RELEASE_MODE; then
        RELEASE_DIR="$PROJECT_ROOT/release"
        mkdir -p "$RELEASE_DIR"

        declare -a ARCHS=("arm64-v8a" "armeabi-v7a" "x86_64" "x86")
        for ARCH in "${ARCHS[@]}"; do
            BIN_PATH="$PROJECT_ROOT/libs/$ARCH/$BINARY_NAME"
            if [ -f "$BIN_PATH" ]; then
                cp "$BIN_PATH" "$RELEASE_DIR/${BINARY_NAME}-${ARCH}"
                echo "Copied $ARCH binary to release folder."
            fi
        done

        echo "Release build complete. Binaries are in $RELEASE_DIR"
        exit 0
    fi

    # Normal non-release mode
    if [ -f "$PROJECT_ROOT/libs/arm64-v8a/$BINARY_NAME" ]; then
        BINARY_PATH="$PROJECT_ROOT/libs/arm64-v8a/$BINARY_NAME"
    elif [ -f "$PROJECT_ROOT/libs/armeabi-v7a/$BINARY_NAME" ]; then
        BINARY_PATH="$PROJECT_ROOT/libs/armeabi-v7a/$BINARY_NAME"
    elif [ -f "$PROJECT_ROOT/libs/x86_64/$BINARY_NAME" ]; then
        BINARY_PATH="$PROJECT_ROOT/libs/x86_64/$BINARY_NAME"
    elif [ -f "$PROJECT_ROOT/libs/x86/$BINARY_NAME" ]; then
        BINARY_PATH="$PROJECT_ROOT/libs/x86/$BINARY_NAME"
    else
        echo "Error: Built binary not found in any architecture subdirectory."
        exit 1
    fi

    echo "Found binary at: $BINARY_PATH"
    
    TERMUX_UID=$(adb shell "pm list packages -U com.termux | cut -d: -f3 | tr -d '\r'")
    
    if [ -z "$TERMUX_UID" ]; then
        echo "Error: Could not find Termux UID. Is Termux installed on the device?"
        exit 1
    fi
    
    echo "Detected Termux UID: $TERMUX_UID"
    
    echo "Pushing binary to Termux..."
    adb push "$(wslpath -w $BINARY_PATH)" "$TERMUX_BIN/$BINARY_NAME"
    
    echo "Setting permissions..."
    adb shell "chmod 755 $TERMUX_BIN/$BINARY_NAME"
    
    echo "Installation complete!"
    echo "Binary installed to: $TERMUX_BIN/$BINARY_NAME"
else
    echo "NDK build failed."
    exit 1
fi
