#!/bin/bash

# --- Configuration ---
# Set the path to your Android NDK installation
# IMPORTANT: Adjust this path if your NDK is installed elsewhere
NDK_ROOT="/home/jimmy/android-ndk-r27c"

# Set the path to your project directory where Android.mk and Application.mk are located
PROJECT_ROOT="$(pwd)"

# Termux installation path
TERMUX_BIN="/data/data/com.termux/files/usr/bin"

BINARY_NAME="psh"

# --- Build Process ---

echo "Starting NDK build for $BINARY_NAME..."
echo "NDK Root: $NDK_ROOT"
echo "Project Root: $PROJECT_ROOT"

# Ensure NDK_ROOT is valid
if [ ! -d "$NDK_ROOT" ]; then
    echo "Error: NDK_ROOT directory '$NDK_ROOT' not found."
    echo "Please update the NDK_ROOT variable in this script to point to your NDK installation."
    exit 1
fi

# Ensure project files exist
if [ ! -f "$PROJECT_ROOT/Android.mk" ] || [ ! -f "$PROJECT_ROOT/Application.mk" ] || [ ! -f "$PROJECT_ROOT/main.cpp" ]; then
    echo "Error: Missing one or more required project files (Android.mk, Application.mk, or main.cpp) in '$PROJECT_ROOT'."
    echo "Please ensure these files are present."
    exit 1
fi

# Run ndk-build
"$NDK_ROOT/ndk-build" NDK_PROJECT_PATH="$PROJECT_ROOT" APP_BUILD_SCRIPT="$PROJECT_ROOT/Android.mk"

# --- Post-Build Steps ---
if [ $? -eq 0 ]; then
    echo "NDK build successful!"
    
    # Find the most appropriate binary (prefer arm64-v8a if available)
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
    
    # Get Termux UID (first user with the com.termux package)
    TERMUX_UID=$(adb shell "pm list packages -U com.termux | cut -d: -f3 | tr -d '\r'")
    
    if [ -z "$TERMUX_UID" ]; then
        echo "Error: Could not find Termux UID. Is Termux installed on the device?"
        exit 1
    fi
    
    echo "Detected Termux UID: $TERMUX_UID"
    
    # Push the binary to Termux's bin directory
    echo "Pushing binary to Termux..."
    adb push "$(wslpath -w $BINARY_PATH)" "$TERMUX_BIN/$BINARY_NAME"
    
    # Set permissions and ownership
    echo "Setting permissions..."
    #adb shell "chown $TERMUX_UID:$TERMUX_UID $TERMUX_BIN/$BINARY_NAME"
    adb shell "chmod 755 $TERMUX_BIN/$BINARY_NAME"
    
    echo "Installation complete!"
    echo "Binary installed to: $TERMUX_BIN/$BINARY_NAME"
else
    echo "NDK build failed. Check the errors above."
    exit 1
fi