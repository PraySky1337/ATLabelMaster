# Changelog

All notable changes to ATLabelMaster will be documented in this file.

## [Unreleased]

### Added - Pixel Art UI System
- **Theme Manager** - Complete theme system with JSON configuration
- **Three Themes**
  - Retro Gaming - 8-bit/16-bit inspired pixel art style
  - Dark Modern - VSCode-style dark theme
  - Classic - Traditional Qt appearance
- **Theme Selector** - Switch themes from Settings dialog
- **Pixel Widgets**
  - PixelButton - Block-style buttons with sharp corners
  - PixelSlider - Pixel-styled slider with square handle
  - PixelCheckBox - Square checkboxes with blocky checkmark
  - PixelRadioButton - Square radio buttons
  - PixelDialog - Base class for pixel-styled dialogs
  - PixelCanvas - Image canvas with pixel-perfect rendering

### Added - Performance & Reliability
- **Image Cache** - LRU cache for efficient image loading
  - Configurable size limits (default: 100MB, 100 images)
  - Thread-safe implementation
  - Cache statistics tracking
- **Crash Handler** - Automatic crash recovery
  - Signal handlers for Unix systems
  - Stack trace capture
  - State persistence for recovery
  - AutoSaveGuard for automatic saves

### Added - User Experience
- **Keyboard Manager** - Customizable keyboard shortcuts
  - 18+ configurable actions
  - Persistence in QSettings
  - Easy reset to defaults
- **Installer Scripts** - Cross-platform installation
  - Universal installer for all Linux distributions
  - Automatic dependency detection
  - Clean uninstall script

### Added - Build System
- **CI/CD Pipeline** - GitHub Actions workflows
  - Build on Ubuntu 22.04, Arch Linux, Fedora 40
  - Automated testing
  - Automated package building on tags
- **Package Builders**
  - Arch Linux PKGBUILD
  - Debian/Ubuntu build script
  - RPM spec file for Fedora/RHEL
  - Unified build script for all packages

### Changed
- Updated CMakeLists.txt to install theme assets
- Enhanced AppSettings with theme persistence
- Updated main.cpp to initialize theme system
- Added theme selector UI in Settings dialog

### Fixed
- Build issues with various Qt6 versions
- Compatibility with GCC 11+ and Clang 13+

### Technical Details
- **New Files**: 30+ new source files
- **Lines of Code**: ~3,000+ new lines
- **Themes**: JSON-based with 20+ color definitions each
- **Cache Size**: Configurable (default 100MB)
- **Supported Distributions**: Arch, Debian/Ubuntu, Fedora/RHEL

---

## [1.2.2] - 2024-01-XX

### Original Features
- AI-assisted armor plate annotation
- Batch label replacement
- Histogram equalization
- Smart detection with OpenVINO
- Multi-format export (RMML compatible)
