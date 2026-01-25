#!/bin/bash
set -e

# Build script for Debian/Ubuntu package
# Usage: ./build.sh [version] [arch]

VERSION=${1:-"1.2.2"}
ARCH=${2:-"amd64"}

echo "Building ATLabelMaster v${VERSION} for Debian/Ubuntu (${ARCH})..."

# Check if running from project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Create package directory structure
PKG_DIR="labelmaster_${VERSION}_${ARCH}"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/share/labelmaster/themes"
mkdir -p "$PKG_DIR/usr/share/labelmaster/icons"
mkdir -p "$PKG_DIR/usr/share/labelmaster/label"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/icons/hicolor/scalable/apps"

# Build the application
mkdir -p build_deb
cd build_deb
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
make -j$(nproc)
make DESTDIR="../$PKG_DIR" install
cd ..

# Copy desktop file
cp packaging/common/labelmaster.desktop "$PKG_DIR/usr/share/applications/"

# Copy icon
cp assets/icons/1.svg "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/labelmaster.svg"

# Generate control file
cat > "$PKG_DIR/DEBIAN/control" << EOF
Package: labelmaster
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: ATLabelMaster Contributors <noreply@example.com>
Depends: libc6, libgcc-s1, libstdc++6, qt6-base-dev, libopencv-dev, libopenvino-dev, libeigen3-dev
Section: graphics
Priority: optional
Homepage: https://github.com/mybna134/ATLabelMaster
Description: RoboMaster 装甲板标注工具 (像素风格版)
 ATLabelMaster is a pixel-art style annotation tool for RoboMaster
 armor plates. Features include:
  - AI-assisted annotation with OpenVINO
  - Multiple pixel art themes (Retro Gaming, Dark Modern, Classic)
  - Batch label replacement
  - Histogram equalization
  - Support for various armor types and colors
EOF

# Generate postinst script
cat > "$PKG_DIR/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database /usr/share/applications
fi

# Update icon cache
if command -v gtk-update-icon-cache &> /dev/null; then
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor &> /dev/null
fi

exit 0
EOF

# Generate prerm script
cat > "$PKG_DIR/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e
exit 0
EOF

# Make scripts executable
chmod 755 "$PKG_DIR/DEBIAN/postinst"
chmod 755 "$PKG_DIR/DEBIAN/prerm"

# Calculate installed size
INSTALLED_SIZE=$(du -sk "$PKG_DIR" | cut -f1)
echo "Installed-Size: $INSTALLED_SIZE" >> "$PKG_DIR/DEBIAN/control"

# Build the package
dpkg-deb --build "$PKG_DIR"

echo "Done! Package built:"
ls -lh "${PKG_DIR}.deb"

# Cleanup (optional)
# rm -rf "$PKG_DIR"
# rm -rf build_deb
