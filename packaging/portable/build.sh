#!/bin/bash
set -e

# Portable package build script for ATLabelMaster
# Creates tar.gz and zip packages with all included assets

VERSION=${1:-"1.2.2"}
ARCH=$(uname -m)
PKG_NAME="ATLabelMaster-${VERSION}-linux-${ARCH}"

echo "Building portable package: ${PKG_NAME}"

# Check if running from project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Count available CPU cores
CORES=$(nproc 2>/dev/null || echo "4")

# Build the application
echo ">>> Building application..."
mkdir -p build_portable
cd build_portable
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j${CORES}
cd ..

# Create package directory structure
echo ">>> Creating package structure..."
rm -rf "${PKG_NAME}"
mkdir -p "${PKG_NAME}"

# Copy executable
cp bin/LabelMaster "${PKG_NAME}/"
chmod +x "${PKG_NAME}/LabelMaster"

# Copy assets directory
cp -r assets/ "${PKG_NAME}/"

# Copy launcher script
cp packaging/portable/wrapper.sh "${PKG_NAME}/LabelMaster.sh"
chmod +x "${PKG_NAME}/LabelMaster.sh"

# Create README
cat > "${PKG_NAME}/README.txt" << 'EOF'
ATLabelMaster - Portable Distribution

=== INSTALLATION ===

No installation required! Simply extract the archive and run:

1. Extract the archive:
   tar -xzf ATLabelMaster-*.tar.gz
   # or unzip ATLabelMaster-*.zip

2. Navigate to the extracted directory:
   cd ATLabelMaster-*

3. Run the application:
   ./LabelMaster.sh

Or run directly:
   ./LabelMaster

=== REQUIREMENTS ===

This portable package requires the following system libraries:
- Qt6 (Widgets, Core, Gui, Svg)
- OpenCV
- OpenVINO
- Eigen3

On Debian/Ubuntu:
  sudo apt install qt6-base-dev libopencv-dev libopenvino-dev eigen3-dev

On Arch Linux:
  sudo pacman -S qt6-base opencv openvino eigen

On Fedora/RHEL:
  sudo dnf install qt6-qtbase opencv-devel openvino-devel eigen3-devel

=== DIRECTORY STRUCTURE ===

LabelMaster          - Main executable
LabelMaster.sh       - Launcher script (recommended)
assets/              - Application assets
  ├── themes/        - UI themes
  ├── icons/         - Application icons
  ├── label/         - Label resources
  └── models/        - ML models

=== TROUBLESHOOTING ===

If the application fails to start:
1. Ensure all dependencies are installed
2. Check file permissions: chmod +x LabelMaster LabelMaster.sh
3. Run with ./LabelMaster.sh for better asset path handling

For more information, visit: https://github.com/mybna134/ATLabelMaster
EOF

# Create tar.gz package
echo ">>> Creating tar.gz package..."
tar -czf "${PKG_NAME}.tar.gz" "${PKG_NAME}"

# Create zip package
echo ">>> Creating zip package..."
zip -qr "${PKG_NAME}.zip" "${PKG_NAME}"

# Clean up
rm -rf "${PKG_NAME}"

echo ""
echo "✓ Portable packages created successfully:"
echo "  - ${PKG_NAME}.tar.gz"
echo "  - ${PKG_NAME}.zip"
echo ""
echo "Installation instructions are included in the package."
