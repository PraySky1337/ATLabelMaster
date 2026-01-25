Name:           labelmaster
Version:        1.2.2
Release:        1%{?dist}
Summary:        RoboMaster Armor Annotation Tool (Pixel Art Style)

License:        MIT
URL:            https://github.com/mybna134/ATLabelMaster
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  opencv-devel
BuildRequires:  openvino-devel
BuildRequires:  eigen3-devel
BuildRequires:  gcc-c++
BuildRequires:  make

Requires:       qt6-qtbase-gui
Requires:       qt6-qtsvg
Requires:       opencv-libs
Requires:       openvino
Requires:       eigen3-libs

%description
ATLabelMaster is a pixel-art style annotation tool for RoboMaster
armor plates. Features include:
- AI-assisted annotation with OpenVINO
- Multiple pixel art themes (Retro Gaming, Dark Modern, Classic)
- Batch label replacement
- Histogram equalization
- Support for various armor types and colors

%prep
%setup -q

%build
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=%{_prefix} \
      ..
make %{?_smp_mflags}

%install
cd build
make DESTDIR=%{buildroot} install

# Install desktop file
install -Dm644 %{_sourcedir}/packaging/common/labelmaster.desktop \
    %{buildroot}%{_datadir}/applications/labelmaster.desktop

# Install icon
install -Dm644 assets/icons/1.svg \
    %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/labelmaster.svg

%files
%{_bindir}/LabelMaster
%{_datadir}/labelmaster/themes/*.json
%{_datadir}/labelmaster/icons/*.svg
%{_datadir}/labelmaster/label/*.json
%{_datadir}/applications/labelmaster.desktop
%{_datadir}/icons/hicolor/scalable/apps/labelmaster.svg
%doc README.md
%license LICENSE

%changelog
* $(date +'%a %b %d %Y') ATLabelMaster Contributors <noreply@example.com> - %{version}-%{release}
- Initial package release with pixel art styling
- Added theme system with Retro Gaming, Dark Modern, and Classic themes
- Arch Linux, Debian, and RPM packaging support
