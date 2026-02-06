#!/bin/bash
set -e

# AppImage build script for ATLabelMaster
# Creates self-contained AppImage with all dependencies bundled

VERSION=${1:-"1.2.2"}
APPDIR="ATLabelMaster.AppDir"

echo "Building AppImage: ATLabelMaster-${VERSION}"

# Check if running from project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Count available CPU cores
CORES=$(nproc 2>/dev/null || echo "4")

# Download linuxdeploy and qt plugin
echo ">>> Downloading linuxdeploy tools..."
if [ ! -f "linuxdeploy" ]; then
    wget -q --show-progress "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O linuxdeploy
    chmod +x linuxdeploy
fi

if [ ! -f "linuxdeploy-plugin-qt" ]; then
    wget -q --show-progress "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -O linuxdeploy-plugin-qt
    chmod +x linuxdeploy-plugin-qt
fi

# Build the application
echo ">>> Building application..."
mkdir -p build_appimage
cd build_appimage
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
make -j${CORES}
make DESTDIR="../${APPDIR}" install
cd ..

# Copy assets to AppDir
echo ">>> Copying assets..."
mkdir -p "${APPDIR}/usr/share/labelmaster/assets"
cp -r assets/* "${APPDIR}/usr/share/labelmaster/assets/"

# Copy AppRun
echo ">>> Setting up AppRun..."
cp packaging/appimage/AppRun "${APPDIR}/"
chmod +x "${APPDIR}/AppRun"

# Copy desktop file
echo ">>> Installing desktop file..."
mkdir -p "${APPDIR}/usr/share/applications"
cp packaging/common/labelmaster.desktop "${APPDIR}/usr/share/applications/"

# Find and copy icon
echo ">>> Installing icon..."
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
if [ -f "assets/icons/labelmaster.png" ]; then
    cp assets/icons/labelmaster.png "${APPDIR}/usr/share/icons/hicolor/256x256/apps/labelmaster.png"
elif [ -f "assets/icons/app.png" ]; then
    cp assets/icons/app.png "${APPDIR}/usr/share/icons/hicolor/256x256/apps/labelmaster.png"
else
    echo "Warning: No icon found in assets/icons/"
fi

# Build AppImage
echo ">>> Building AppImage..."
export VERSION="${VERSION}"
export QMAKE="qmake6"
./linuxdeploy --appdir="${APPDIR}" --plugin=qt --output-appimage

# Rename AppImage to include version
if [ -f "ATLabelMaster-x86_64.AppImage" ]; then
    mv "ATLabelMaster-x86_64.AppImage" "ATLabelMaster-${VERSION}-x86_64.AppImage"
    echo ""
    echo "✓ AppImage created successfully: ATLabelMaster-${VERSION}-x86_64.AppImage"
else
    echo "⚠ AppImage build may have failed - check output above"
fi

# Clean up
rm -rf "${APPDIR}"

echo ""
echo "To run the AppImage:"
echo "  chmod +x ATLabelMaster-${VERSION}-x86_64.AppImage"
echo "  ./ATLabelMaster-${VERSION}-x86_64.AppImage"
