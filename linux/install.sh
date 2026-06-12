#!/bin/bash
# ============================================================
# Reproductor App - Linux Installer
# Installs the music player binary, desktop entry and icon
# Usage: ./install.sh [--prefix=/usr/local] [--uninstall]
# ============================================================

set -euo pipefail

PREFIX="/usr/local"
UNINSTALL=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix=*)
            PREFIX="${1#--prefix=}"
            shift
            ;;
        --uninstall)
            UNINSTALL=1
            shift
            ;;
        *)
            echo "Usage: $0 [--prefix=/usr/local] [--uninstall]"
            exit 1
            ;;
    esac
done

BINDIR="$PREFIX/bin"
APPDIR="$PREFIX/share/applications"
ICONDIR="$PREFIX/share/icons/hicolor/scalable/apps"

SOURCE_DIR="$(cd "$(dirname "$0")" && pwd)"
DESKTOP_SRC="$SOURCE_DIR/reproductor.desktop"
ICON_SRC="$SOURCE_DIR/reproductor.svg"
PYTHON_SRC="$SOURCE_DIR/player.py"
C_SRC="$SOURCE_DIR/player.c"

# ── uninstall mode ────────────────────────────────────
if [[ $UNINSTALL -eq 1 ]]; then
    echo "==> Uninstalling Reproductor App..."
    rm -f "$BINDIR/reproductor"
    rm -f "$APPDIR/reproductor.desktop"
    rm -f "$ICONDIR/reproductor.svg"
    gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" 2>/dev/null || true
    echo "    Done."
    exit 0
fi

# ── root check ────────────────────────────────────────
if [[ "$PREFIX" != "$HOME" ]] && [[ "$PREFIX" != "$HOME"/* ]] && [[ "$EUID" -ne 0 ]]; then
    echo "==> Re-running with sudo for system-wide install..."
    exec sudo "$0" --prefix="$PREFIX"
fi

# ── detect which source to use ────────────────────────
USE_PYTHON=0
if [[ -f "$C_SRC" ]]; then
    USE_PYTHON=0
elif [[ -f "$PYTHON_SRC" ]]; then
    USE_PYTHON=1
else
    echo "ERROR: No se encontro player.c ni player.py"
    exit 1
fi

BINARY="$SOURCE_DIR/reproductor"

# ── detect package manager ────────────────────────────
PM=""
if command -v apt &>/dev/null; then
    PM="apt"
elif command -v dnf &>/dev/null; then
    PM="dnf"
elif command -v pacman &>/dev/null; then
    PM="pacman"
fi

if [[ $USE_PYTHON -eq 0 ]]; then
    echo "==> Checking build dependencies..."

    MISSING_BUILD=()

    for triple in \
        "gtk+-3.0:libgtk-3-dev:gtk3-devel:gtk3" \
        "gstreamer-1.0:libgstreamer1.0-dev:gstreamer1-devel:gstreamer" \
        "gstreamer-pbutils-1.0:libgstreamer-plugins-base1.0-dev:gstreamer1-plugins-base-devel:gst-plugins-base-libs" \
    ; do
        IFS=':' read -r pc_check apt_pkg dnf_pkg pacman_pkg <<< "$triple"
        if ! pkg-config --exists "$pc_check" 2>/dev/null; then
            case "$PM" in
                apt)   MISSING_BUILD+=("$apt_pkg") ;;
                dnf)   MISSING_BUILD+=("$dnf_pkg") ;;
                pacman) MISSING_BUILD+=("$pacman_pkg") ;;
            esac
        fi
    done

    if ! command -v gcc &>/dev/null; then
        case "$PM" in
            apt)   MISSING_BUILD+=("build-essential") ;;
            dnf)   MISSING_BUILD+=("gcc pkgconfig make") ;;
            pacman) MISSING_BUILD+=("base-devel") ;;
        esac
    fi

    if [[ -n "$PM" ]]; then
        # Also add runtime GStreamer plugins if possible
        case "$PM" in
            apt)   MISSING_BUILD+=("gstreamer1.0-plugins-good" "gstreamer1.0-plugins-bad" "gstreamer1.0-plugins-ugly") ;;
            dnf)   MISSING_BUILD+=("gstreamer1-plugins-good" "gstreamer1-plugins-bad-free" "gstreamer1-plugins-ugly-free") ;;
            pacman) MISSING_BUILD+=("gst-plugins-good" "gst-plugins-bad" "gst-plugins-ugly") ;;
        esac
    fi

    if [[ ${#MISSING_BUILD[@]} -gt 0 ]] && [[ -n "$PM" ]]; then
        echo "==> Installing missing dependencies..."
        case "$PM" in
            apt)
                apt update -qq
                apt install -y "${MISSING_BUILD[@]}"
                ;;
            dnf)
                dnf install -y "${MISSING_BUILD[@]}"
                ;;
            pacman)
                pacman -Sy --noconfirm --needed "${MISSING_BUILD[@]}"
                ;;
        esac
    fi

    # ── compile ───────────────────────────────────────────
    echo "==> Compiling..."
    CFLAGS=$(pkg-config --cflags gtk+-3.0 gstreamer-1.0 gstreamer-pbutils-1.0)
    LIBS=$(pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-pbutils-1.0)

    gcc -Wall -O2 -o "$BINARY" "$C_SRC" $CFLAGS $LIBS -lm

    if [[ ! -f "$BINARY" ]]; then
        echo "ERROR: Compilation failed."
        exit 1
    fi
    echo "    Compiled: $BINARY"
else
    echo "==> Preparando reproductor Python..."

    PYTHON=$(command -v python3)
    if [[ -z "$PYTHON" ]]; then
        echo "WARNING: python3 no encontrado, intentando python..."
        PYTHON=$(command -v python)
    fi
    if [[ -z "$PYTHON" ]]; then
        echo "ERROR: No se encontro el interprete de Python."
        exit 1
    fi

    cat > "$BINARY" << WRAPPER
#!/bin/bash
exec "$PYTHON" "$PYTHON_SRC" "\$@"
WRAPPER
    chmod +x "$BINARY"
    echo "    Wrapper: $BINARY -> $PYTHON_SRC"
fi

# ── check required files ──────────────────────────────
if [[ ! -f "$DESKTOP_SRC" ]]; then
    echo "WARNING: No se encontro $DESKTOP_SRC"
fi
if [[ ! -f "$ICON_SRC" ]]; then
    echo "WARNING: No se encontro $ICON_SRC"
fi

# ── install ───────────────────────────────────────────
echo "==> Installing to $PREFIX..."

mkdir -p "$BINDIR" "$APPDIR" "$ICONDIR"

cp "$BINARY" "$BINDIR/reproductor"
echo "    Binary: $BINDIR/reproductor"

if [[ -f "$DESKTOP_SRC" ]]; then
    cp "$DESKTOP_SRC" "$APPDIR/reproductor.desktop"
    echo "    Desktop: $APPDIR/reproductor.desktop"
fi

if [[ -f "$ICON_SRC" ]]; then
    cp "$ICON_SRC" "$ICONDIR/reproductor.svg"
    echo "    Icon: $ICONDIR/reproductor.svg"
fi

gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" 2>/dev/null || true
echo "    Icon cache updated"

echo ""
echo "==> Installed successfully."
echo "    Launch from application menu or run: reproductor"
echo "    To uninstall: $0 --uninstall"
