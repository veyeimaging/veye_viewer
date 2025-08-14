#!/bin/bash

CONFIG_DIR="./config"
PLATFORM_FILE="$CONFIG_DIR/platform.json"
CAMERA_JSON="$CONFIG_DIR/auto_camera_index.json"

cd "$(dirname "$(realpath "$0")")"

mkdir -p "$CONFIG_DIR"

# 1. Detect platform via /proc/device-tree/compatible
platform="unknown"
if [ -f /proc/device-tree/compatible ]; then
    compat=$(tr -d '\0' < /proc/device-tree/compatible)
    if echo "$compat" | grep -qi "rockchip"; then
        platform="rockchip"
    elif echo "$compat" | grep -qi "nvidia"; then
        platform="jetson"
    elif echo "$compat" | grep -qi "raspberrypi"; then
        if echo "$compat" | grep -qi "raspberrypi,5"; then
            platform="raspberrypi5"
        else
            platform="raspberrypi"
        fi
    fi
fi

echo "{\"platform\": \"$platform\"}" > "$PLATFORM_FILE"
echo "Platform detected: $platform"

# 2. Ensure current user is in the i2c group
current_user=$(whoami)
if ! groups "$current_user" | grep -qw i2c; then
    echo "User $current_user is not in 'i2c' group. Adding now..."
    sudo usermod -aG i2c "$current_user"
    newgrp i2c #change to i2c group now
    echo "User added to i2c group. Please re-login or reboot for this to take effect."
fi

# 3. Rockchip-specific: Check if gawk is installed
if [ "$platform" = "rockchip" ]; then
    if ! command -v gawk >/dev/null 2>&1; then
        echo "Missing required tool: gawk"
        echo "Please install gawk using your package manager, e.g.:"
        echo "  sudo apt install gawk"
        exit 1
    fi
fi

# 4. Check if Qt5 is installed (look for qt5 libraries or qmake-qt5)
if ! command -v qmake-qt5 >/dev/null 2>&1 && ! dpkg -l | grep -q "qt5"; then
    echo "Qt5 is not installed. Please install it using:"
    echo "  sudo apt install qt5-default"
	echo "or:"
	echo "  sudo apt install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools"
    exit 1
fi

# Check if v4l-utils is installed (look for v4l2-ctl)
if ! command -v v4l2-ctl >/dev/null 2>&1 && ! dpkg -l | grep -q "v4l-utils"; then
    echo "v4l-utils is not installed. Please install it using:"
    echo "  sudo apt install v4l-utils"
    exit 1
fi

# Check if gawk is installed
if ! command -v gawk >/dev/null 2>&1 && ! dpkg -l | grep -q "gawk"; then
    echo "gawk is not installed. Please install it using:"
    echo "  sudo apt install gawk"
    exit 1
fi

# 5. Check if auto_camera_index.json exists
if [ ! -f "$CAMERA_JSON" ]; then
    echo "Camera index file not found. Running platform-specific probe script..."
    case "$platform" in
        rockchip)
            if [ -f ./$CONFIG_DIR/probe_camera_info-rk.sh ]; then
                bash ./$CONFIG_DIR/probe_camera_info-rk.sh
            else
                echo "Missing probe script: probe_camera_info-rk.sh"
                exit 1
            fi
            ;;
        jetson)
            if [ -f ./$CONFIG_DIR/probe_camera_info-jetson.sh ]; then
                bash ./$CONFIG_DIR/probe_camera_info-jetson.sh
            else
                echo "Missing probe script: probe_camera_info-jetson.sh"
                exit 1
            fi
            ;;
        raspberrypi)
            if [ -f ./$CONFIG_DIR/probe_camera_info-rpi.sh ]; then
                bash ./$CONFIG_DIR/probe_camera_info-rpi.sh
            else
                echo "Missing probe script: probe_camera_info-rpi.sh"
                exit 1
            fi
            ;;
        raspberrypi5)
            if [ -f ./$CONFIG_DIR/probe_camera_info-rpi5.sh ]; then
                bash ./$CONFIG_DIR/probe_camera_info-rpi5.sh
            else
                echo "Missing probe script: probe_camera_info-rpi.sh"
                exit 1
            fi
            ;;
        *)
            echo "Unknown platform. Cannot select probe script."
            exit 1
            ;;
    esac
fi

# 6. Launch veye_viewer
if [ -x ./veye_viewer ]; then
    echo "Launching veye_viewer..."
    ./veye_viewer
else
    echo "veye_viewer binary not found or not executable."
    exit 1
fi
