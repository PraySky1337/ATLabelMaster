#!/bin/bash
# ATLabelMaster Installer Script
# Supports: Arch Linux, Debian/Ubuntu, Fedora/RHEL, Generic binary install

set -e

VERSION="1.2.2"
INSTALL_DIR="/opt/labelmaster"
BIN_LINK="/usr/local/bin/labelmaster"
DESKTOP_LINK="/usr/share/applications/labelmaster.desktop"

echo "=================================="
echo "ATLabelMaster v${VERSION} Installer"
echo "=================================="
echo ""

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION_ID=$VERSION_ID
else
    DISTRO="unknown"
fi

echo "Detected distribution: $DISTRO"

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Function to install on Arch Linux
install_arch() {
    echo "Installing on Arch Linux..."

    if ! pacman -Q qt6-base >/dev/null 2>&1; then
        echo "Installing dependencies..."
        pacman -S --needed --noconfirm qt6-base qt6-svg opencv openvino eigen cmake git
    fi

    echo "Building package..."
    bash packaging/arch/build.sh "$VERSION"

    echo "Installing package..."
    pacman -U --noconfirm labelmaster-*.pkg.tar.zst

    echo "Cleaning up..."
    rm -f labelmaster-*.pkg.tar.zst
}

# Function to install on Debian/Ubuntu
install_debian() {
    echo "Installing on Debian/Ubuntu..."

    if ! dpkg -l | grep -q qt6-base-dev; then
        echo "Installing dependencies..."
        apt-get update
        apt-get install -y qt6-base-dev qt6-base-dev-tools libqt6svg6-dev \
            libopencv-dev libopenvino-dev libeigen3-dev cmake git build-essential
    fi

    echo "Building package..."
    bash packaging/deb/build.sh "$VERSION" "amd64"

    echo "Installing package..."
    dpkg -i labelmaster_*.deb

    echo "Cleaning up..."
    rm -f labelmaster_*.deb
}

# Function to install on Fedora/RHEL
install_fedora() {
    echo "Installing on Fedora/RHEL..."

    if ! rpm -qa | grep -q qt6-qtbase-devel; then
        echo "Installing dependencies..."
        dnf install -y qt6-qtbase-devel qt6-qtsvg-devel opencv-devel \
            openvino-devel eigen3-devel cmake git gcc-c++
    fi

    echo "Building package..."
    bash packaging/rpm/build.sh "$VERSION"

    echo "Installing package..."
    dnf install -y labelmaster-*.rpm

    echo "Cleaning up..."
    rm -f labelmaster-*.rpm
}

# Function to install from pre-built binary
install_binary() {
    echo "Installing from pre-built binary..."

    # Build if not already built
    if [ ! -f "bin/LabelMaster" ]; then
        echo "Building binary..."
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
        make -j$(nproc)
        cd ..
    fi

    # Create install directory
    mkdir -p "$INSTALL_DIR"

    # Copy binary
    cp bin/LabelMaster "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/LabelMaster"

    # Copy assets
    mkdir -p "$INSTALL_DIR/assets"
    cp -r assets/themes "$INSTALL_DIR/assets/"
    cp -r assets/icons "$INSTALL_DIR/assets/"
    cp -r assets/label "$INSTALL_DIR/assets/"
    cp -r assets/models "$INSTALL_DIR/assets/" 2>/dev/null || true

    # Copy desktop file
    mkdir -p /usr/share/applications
    cp packaging/common/labelmaster.desktop /usr/share/applications/

    # Copy icon
    mkdir -p /usr/share/icons/hicolor/scalable/apps
    cp assets/icons/1.svg /usr/share/icons/hicolor/scalable/apps/labelmaster.svg

    # Create symlink
    ln -sf "$INSTALL_DIR/LabelMaster" "$BIN_LINK"

    # Update desktop database
    update-desktop-database /usr/share/applications 2>/dev/null || true
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
}

# Main installation logic
case "$DISTRO" in
    arch|manjaro|endeavouros|garuda)
        install_arch
        ;;
    ubuntu|debian|linuxmint|pop)
        install_debian
        ;;
    fedora|rhel|centos|rocky|almalinux)
        install_fedora
        ;;
    *)
        echo "Unsupported distribution. Installing from binary..."
        install_binary
        ;;
esac

echo ""
echo "=================================="
echo "Installation complete!"
echo "=================================="
echo ""
echo "Launch ATLabelMaster with: labelmaster"
echo "Or from your application menu."
echo ""
echo "To uninstall:"
case "$DISTRO" in
    arch|manjaro|endeavouros|garuda)
        echo "  sudo pacman -R labelmaster"
        ;;
    ubuntu|debian|linuxmint|pop)
        echo "  sudo apt remove labelmaster"
        ;;
    fedora|rhel|centos|rocky|almalinux)
        echo "  sudo dnf remove labelmaster"
        ;;
    *)
        echo "  sudo rm -rf $INSTALL_DIR $BIN_LINK"
        ;;
esac
echo ""
