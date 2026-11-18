#!/bin/bash
#
# veye_viewer dependency installer
#
# Purpose: replace the "manually apt-install one package at a time" first-time
#          setup with a single command. It detects the platform, installs the
#          required system packages, and grants executable bits to the
#          start/probe scripts.
#
# Usage:
#   ./setup.sh              Default: install RUNTIME dependencies only
#   ./setup.sh --build      Also install the build/compile toolchain
#   ./setup.sh --ide        Also install Qt Creator
#   ./setup.sh --no-i2c     Skip adding the current user to the i2c group
#   ./setup.sh --help       Show help
#
# Note: only apt-based distros are covered (Ubuntu / Debian / Raspbian /
#       NVIDIA L4T / common Rockchip images). Other package managers print a
#       reminder of the package list and exit.

set -uo pipefail

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
WANT_BUILD=0
WANT_IDE=0
WANT_I2C=1
SHOW_HELP=0

for arg in "$@"; do
    case "$arg" in
        --run)      : ;;   # runtime deps are always installed; kept for clarity
        --build)    WANT_BUILD=1 ;;
        --ide)      WANT_IDE=1 ;;
        --no-i2c)   WANT_I2C=0 ;;
        -h|--help)  SHOW_HELP=1 ;;
        *)
            echo "Unknown argument: $arg" >&2
            SHOW_HELP=1
            ;;
    esac
done

if [ "$SHOW_HELP" -eq 1 ]; then
    cat <<'EOF'
veye_viewer dependency installer

Usage: ./setup.sh [options...]

  (no option)  Install runtime dependencies only (recommended default)
  --build      Additionally install the build/compile toolchain
               (Qt5 dev headers, cmake, g++, make, gdb, lupdate/lrelease)
  --ide        Additionally install Qt Creator
  --no-i2c     Do not add the current user to the i2c group
  -h, --help   Show this help

Examples:
  ./setup.sh                 # first-time install, just run the app
  ./setup.sh --build         # also set up the environment to compile from source
EOF
    exit 0
fi

# ---------------------------------------------------------------------------
# Colored output (auto-disabled when not a TTY, to keep logs clean)
# ---------------------------------------------------------------------------
if [ -t 1 ]; then
    C_RED=$'\033[31m'; C_GREEN=$'\033[32m'; C_YELLOW=$'\033[33m'; C_BLUE=$'\033[36m'; C_BOLD=$'\033[1m'; C_OFF=$'\033[0m'
else
    C_RED=''; C_GREEN=''; C_YELLOW=''; C_BLUE=''; C_BOLD=''; C_OFF=''
fi
info()  { echo "${C_BLUE}[INFO]${C_OFF} $*"; }
ok()    { echo "${C_GREEN}[ OK ]${C_OFF} $*"; }
warn()  { echo "${C_YELLOW}[WARN]${C_OFF} $*"; }
err()   { echo "${C_RED}[FAIL]${C_OFF} $*" >&2; }

# ---------------------------------------------------------------------------
# sudo handling: not needed as root; use sudo when available; otherwise abort
# ---------------------------------------------------------------------------
SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    if command -v sudo >/dev/null 2>&1; then
        SUDO="sudo"
    else
        err "Not running as root and no sudo found. Please re-run as root."
        exit 1
    fi
fi

# ---------------------------------------------------------------------------
# Detect package manager (only apt is supported for now)
# ---------------------------------------------------------------------------
if command -v apt-get >/dev/null 2>&1 || command -v apt >/dev/null 2>&1; then
    PM="apt"
else
    PM="unknown"
fi

if [ "$PM" != "apt" ]; then
    err "apt package manager not detected."
    err "This script currently supports Debian/Ubuntu families only; on other"
    err "systems please install the following packages manually:"
    echo "  Runtime: gawk v4l-utils libqt5opengl5 libqt5widgets5 libqt5gui5 libqt5core5a"
    [ "$WANT_BUILD" -eq 1 ] && echo "  Build:   cmake g++ make gdb qtbase5-dev qttools5-dev qttools5-dev-tools libqt5opengl5-dev"
    [ "$WANT_IDE" -eq 1 ]   && echo "  IDE:     qtcreator"
    exit 1
fi

# ---------------------------------------------------------------------------
# Package lists
#
# Runtime:
#   gawk           - used by config/probe_camera_info-*.sh to parse the media graph
#   v4l-utils      - provides media-ctl / v4l2-ctl called by the probe scripts
#   libqt5*        - Qt5 runtime (the app is a Qt5 Widgets/OpenGL program)
#
# Build (matches CMakeLists.txt: find_package Qt COMPONENTS Widgets OpenGL LinguistTools):
#   qtbase5-dev           - Qt5 Core/Gui/Widgets
#   libqt5opengl5-dev     - Qt5 OpenGL
#   qttools5-dev(-tools)  - lupdate / lrelease (compile config/*.ts translations)
#   cmake g++ make gdb    - build chain and debugger
# ---------------------------------------------------------------------------
RUN_DEPS=(gawk v4l-utils libqt5opengl5 libqt5widgets5 libqt5gui5 libqt5core5a)
BUILD_DEPS=(cmake g++ make gdb qtbase5-dev qttools5-dev qttools5-dev-tools libqt5opengl5-dev)
IDE_DEPS=(qtcreator)

PACKAGES=("${RUN_DEPS[@]}")
[ "$WANT_BUILD" -eq 1 ] && PACKAGES+=("${BUILD_DEPS[@]}")
[ "$WANT_IDE" -eq 1 ]   && PACKAGES+=("${IDE_DEPS[@]}")

# de-duplicate
mapfile -t PACKAGES < <(printf '%s\n' "${PACKAGES[@]}" | awk '!seen[$0]++')

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
info "Refreshing apt package index ..."
if ! $SUDO apt-get update; then
    warn "apt-get update failed; continuing with possibly cached indexes."
fi

info "Installing packages: ${PACKAGES[*]}"
# DEBIAN_FRONTEND=noninteractive avoids interactive popups (e.g. popconf) hanging the run
if $SUDO env DEBIAN_FRONTEND=noninteractive apt-get install -y "${PACKAGES[@]}"; then
    ok "Dependencies installed."
else
    err "Some packages failed to install; check the apt output above (network / sources / package names)."
    exit 1
fi

# ---------------------------------------------------------------------------
# Grant executable bits to start/probe scripts (the app execs them directly)
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ -f ./start_veye_viewer.sh ]; then
    chmod +x ./start_veye_viewer.sh
    ok "start_veye_viewer.sh set executable."
fi
if compgen -G "./config/*.sh" >/dev/null; then
    chmod 755 ./config/*.sh
    ok "config/*.sh set executable."
fi

# ---------------------------------------------------------------------------
# Add current user to the i2c group (needed to access /dev/i2c-*, otherwise the
# on-screen log reports permission errors)
# ---------------------------------------------------------------------------
if [ "$WANT_I2C" -eq 1 ]; then
    current_user="${SUDO_USER:-$(whoami)}"
    if getent group i2c >/dev/null 2>&1; then
        if id -nG "$current_user" | grep -qw i2c; then
            ok "User $current_user is already in the i2c group."
        else
            info "Adding user $current_user to the i2c group ..."
            if $SUDO usermod -aG i2c "$current_user"; then
                ok "Added. ${C_BOLD}Re-login (or reboot) for it to take effect.${C_OFF}"
            else
                warn "Failed to add to i2c group; you can run later: sudo usermod -aG i2c $current_user"
            fi
        fi
    else
        warn "No 'i2c' group on this system; skipped. If you hit i2c permission issues: sudo chmod 777 /dev/i2c-*"
    fi
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo
echo "${C_BOLD}Setup complete.${C_OFF}"
echo "Next steps:"
echo "  Run:     ./start_veye_viewer.sh"
if [ "$WANT_BUILD" -eq 1 ]; then
    echo "  Compile: mkdir -p build && cd build && cmake .. && make"
else
    echo "  (Add --build to this script if you also want to compile from source.)"
fi
echo "  If the first run reports no i2c permission, re-login for the i2c group to take effect, or run: sudo chmod 777 /dev/i2c-*"
