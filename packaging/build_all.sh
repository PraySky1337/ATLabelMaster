#!/bin/bash
set -e

# Unified build script for all package formats
# Usage: ./build_all.sh [version]

VERSION=${1:-"1.2.2"}
START_TIME=$(date +%s)

echo "==================================="
echo "ATLabelMaster Package Builder v${VERSION}"
echo "==================================="
echo ""

# Check if running from project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Count available CPU cores
CORES=$(nproc 2>/dev/null || echo "4")

echo "Building on ${CORES} cores"
echo ""

# Build binary first
echo ">>> Step 1: Building application binary..."
mkdir -p build_all
cd build_all
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
make -j${CORES}
cd ..
echo "✓ Binary build complete"
echo ""

# Build Debian package
echo ">>> Step 2: Building Debian/Ubuntu package..."
if bash packaging/deb/build.sh "$VERSION" "amd64"; then
    echo "✓ Debian package build complete"
else
    echo "⚠ Debian package build failed (this is expected on non-Debian systems)"
fi
echo ""

# Build Arch Linux package
echo ">>> Step 3: Building Arch Linux package..."
if command -v makepkg &> /dev/null; then
    if bash packaging/arch/build.sh "$VERSION"; then
        echo "✓ Arch package build complete"
    else
        echo "⚠ Arch package build failed"
    fi
else
    echo "⊘ makepkg not found, skipping Arch package"
fi
echo ""

# Build portable packages
echo ">>> Step 4: Building portable packages (tar.gz + zip)..."
if bash packaging/portable/build.sh "$VERSION"; then
    echo "✓ Portable packages build complete"
else
    echo "⚠ Portable packages build failed"
fi
echo ""

# Build AppImage
echo ">>> Step 5: Building AppImage..."
if bash packaging/appimage/build.sh "$VERSION"; then
    echo "✓ AppImage build complete"
else
    echo "⚠ AppImage build failed"
fi
echo ""

# Build Arch Linux package
echo ">>> Step 6: Building Arch Linux package..."
if command -v makepkg &> /dev/null; then
    if bash packaging/arch/build.sh "$VERSION"; then
        echo "✓ Arch package build complete"
    else
        echo "⚠ Arch package build failed"
    fi
else
    echo "⊘ makepkg not found, skipping Arch package"
fi
echo ""

# Build RPM package
echo ">>> Step 7: Building RPM package..."
if command -v rpmbuild &> /dev/null; then
    if bash packaging/rpm/build.sh "$VERSION"; then
        echo "✓ RPM package build complete"
    else
        echo "⚠ RPM package build failed"
    fi
else
    echo "⊘ rpmbuild not found, skipping RPM package"
fi
echo ""

# Summary
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo "==================================="
echo "Build Summary (completed in ${DURATION}s)"
echo "==================================="

echo ""
echo "Built packages:"
ls -lh *.tar.gz *.zip *.AppImage *.deb *.rpm *.pkg.tar.zst 2>/dev/null || echo "  (No packages built - check build tools)"

echo ""
echo "Installation commands:"
echo "  Portable:      tar -xzf ATLabelMaster-*.tar.gz && cd ATLabelMaster-* && ./LabelMaster.sh"
echo "  AppImage:      chmod +x ATLabelMaster-*.AppImage && ./ATLabelMaster-*.AppImage"
echo "  Debian/Ubuntu: sudo dpkg -i labelmaster_*.deb"
echo "  Arch Linux:    sudo pacman -U labelmaster-*.pkg.tar.zst"
echo "  Fedora/RHEL:   sudo dnf install labelmaster-*.rpm"
echo "  openSUSE:      sudo zypper install labelmaster-*.rpm"
echo ""
