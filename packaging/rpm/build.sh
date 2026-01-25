#!/bin/bash
set -e

# Build script for RPM package (Fedora/RHEL/openSUSE)
# Usage: ./build.sh [version]

VERSION=${1:-"1.2.2"}
SPEC_FILE="labelmaster.spec"

echo "Building ATLabelMaster v${VERSION} as RPM..."

# Check if running from project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Check for rpmbuild
if ! command -v rpmbuild &> /dev/null; then
    echo "Error: rpmbuild not found. Please install rpm-build package:"
    echo "  Fedora/RHEL: sudo dnf install rpm-build"
    echo "  openSUSE: sudo zypper install rpm-build"
    exit 1
fi

# Create rpmbuild directory structure
mkdir -p ~/rpmbuild/{SOURCES,SPECS,SRPMS,RPMS,BUILD,BUILDROOT}

# Create source tarball
echo "Creating source tarball..."
tar --transform "s,^,labelmaster-${VERSION}/," \
    --exclude-vcs \
    --exclude='build*' \
    --exclude='bin' \
    --exclude='.git' \
    -czf ~/rpmbuild/SOURCES/labelmaster-${VERSION}.tar.gz .

# Copy spec file
cp packaging/rpm/$SPEC_FILE ~/rpmbuild/SPECS/

# Build RPM
echo "Building RPM package..."
rpmbuild -ba ~/rpmbuild/SPECS/$SPEC_FILE

# Find and copy the built RPM to current directory
echo "Built packages:"
find ~/rpmbuild/RPMS -name "labelmaster-*.rpm" -exec cp {} ./ \; -exec ls -lh {} \;
find ~/rpmbuild/SRPMS -name "labelmaster-*.src.rpm" -exec cp {} ./ \; -exec ls -lh {} \; 2>/dev/null || true

echo "Done!"
