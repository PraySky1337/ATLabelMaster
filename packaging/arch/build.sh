#!/bin/bash
set -e

# Build script for Arch Linux package
# Usage: ./build.sh [version]

VERSION=${1:-"1.2.2"}
echo "Building ATLabelMaster v${VERSION} for Arch Linux..."

# Check if running from project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Create temporary build directory
BUILD_DIR="build_arch_pkg"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Copy PKGBUILD and common files
cp packaging/arch/PKGBUILD "$BUILD_DIR/"
cp packaging/common/labelmaster.desktop "$BUILD_DIR/"

# Create source tarball (git archive)
echo "Creating source tarball..."
git archive --format=tar --prefix="labelmaster-${VERSION}/" HEAD | gzip > "$BUILD_DIR/labelmaster-${VERSION}.tar.gz"

# Update PKGBUILD with correct source path
sed -i "s|source=.*|source=(\"labelmaster-\${pkgver}.tar.gz\")|" "$BUILD_DIR/PKGBUILD"
sed -i '/source=/!b;n;c\  md5sums=("SKIP")' "$BUILD_DIR/PKGBUILD"

cd "$BUILD_DIR"

# Generate checksums
updpkgsums

# Build package
echo "Building package with makepkg..."
makepkg -sf

# Copy result to parent directory
cp *.pkg.tar.zst ../

echo "Done! Package built:"
ls -lh ../labelmaster-*.pkg.tar.zst

# Cleanup
cd ..
# Uncomment to keep build directory for inspection
# rm -rf "$BUILD_DIR"
