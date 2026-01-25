#!/bin/bash
# ATLabelMaster Uninstaller Script

set -e

echo "=================================="
echo "ATLabelMaster Uninstaller"
echo "=================================="
echo ""

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    DISTRO="unknown"
fi

# Function to uninstall on Arch Linux
uninstall_arch() {
    if pacman -Q labelmaster >/dev/null 2>&1; then
        echo "Removing package..."
        pacman -R --noconfirm labelmaster
    else
        echo "Package not installed via pacman."
        echo "Removing files manually..."
        rm -rf /opt/labelmaster
        rm -f /usr/local/bin/labelmaster
        rm -f /usr/share/applications/labelmaster.desktop
        rm -f /usr/share/icons/hicolor/scalable/apps/labelmaster.svg
    fi
}

# Function to uninstall on Debian/Ubuntu
uninstall_debian() {
    if dpkg -l | grep -q labelmaster; then
        echo "Removing package..."
        apt-get remove --purge -y labelmaster
        apt-get autoremove -y
    else
        echo "Package not installed via dpkg."
        echo "Removing files manually..."
        rm -rf /opt/labelmaster
        rm -f /usr/local/bin/labelmaster
        rm -f /usr/share/applications/labelmaster.desktop
        rm -f /usr/share/icons/hicolor/scalable/apps/labelmaster.svg
    fi
}

# Function to uninstall on Fedora/RHEL
uninstall_fedora() {
    if rpm -qa | grep -q labelmaster; then
        echo "Removing package..."
        dnf remove -y labelmaster
    else
        echo "Package not installed via rpm."
        echo "Removing files manually..."
        rm -rf /opt/labelmaster
        rm -f /usr/local/bin/labelmaster
        rm -f /usr/share/applications/labelmaster.desktop
        rm -f /usr/share/icons/hicolor/scalable/apps/labelmaster.svg
    fi
}

# Function to uninstall manual/binary installation
uninstall_manual() {
    echo "Removing files..."
    rm -rf /opt/labelmaster
    rm -f /usr/local/bin/labelmaster
    rm -f /usr/share/applications/labelmaster.desktop
    rm -f /usr/share/icons/hicolor/scalable/apps/labelmaster.svg
}

# Main uninstallation logic
case "$DISTRO" in
    arch|manjaro|endeavouros|garuda)
        uninstall_arch
        ;;
    ubuntu|debian|linuxmint|pop)
        uninstall_debian
        ;;
    fedora|rhel|centos|rocky|almalinux)
        uninstall_fedora
        ;;
    *)
        echo "Unknown distribution. Removing files manually..."
        uninstall_manual
        ;;
esac

# Update desktop database
update-desktop-database /usr/share/applications 2>/dev/null || true
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

echo ""
echo "Uninstallation complete!"
echo "Note: User configuration files in ~/.config/LabelMaster were preserved."
echo "To remove them, run: rm -rf ~/.config/LabelMaster"
echo ""
