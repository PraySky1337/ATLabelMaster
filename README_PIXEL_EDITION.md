# ATLabelMaster - Pixel Art Edition

<div align="center">

[![Build Status](https://github.com/mybna134/ATLabelMaster/workflows/Build%20and%20Test/badge.svg)](https://github.com/mybna134/ATLabelMaster/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Qt](https://img.shields.io/badge/Qt-6.0+-41CD52?logo=qt)](https://www.qt.io)
[![C++20](https://img.shields.io/badge/C++-20-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)

**像素风格的 RoboMaster 装甲板标注工具**

A pixel-art styled annotation tool for RoboMaster armor plates with AI-assisted detection.

[Features](#features) • [Installation](#installation) • [Usage](#usage) • [Themes](#themes) • [Building](#building)

</div>

---

## Features

### Core Functionality
- **AI-Assisted Annotation** - Smart armor plate detection using OpenVINO
- **Batch Processing** - Replace labels across multiple files at once
- **Histogram Equalization** - Automatic image enhancement
- **Multiple Export Formats** - Compatible with RMML format
- **Keyboard Shortcuts** - Customizable shortcuts for efficient workflow

### Pixel Art Styling (New!)
- **Retro Gaming Theme** - 8-bit/16-bit inspired pixel art UI
- **Dark Modern Theme** - VSCode-style dark theme
- **Classic Theme** - Traditional Qt appearance
- **Pixel-Perfect Widgets** - Custom buttons, sliders, checkboxes, radio buttons
- **Theme Switcher** - Change themes from Settings (requires restart)

### Advanced Features
- **Crash Recovery** - Automatic state saving and recovery
- **Image Cache** - LRU cache for better performance
- **Keyboard Customization** - Fully customizable keyboard shortcuts
- **Cross-Platform** - Linux support with multiple package formats

---

## Installation

### Arch Linux
```bash
# Build and install from AUR (coming soon)
# Or manually:
git clone https://github.com/mybna134/ATLabelMaster.git
cd ATLabelMaster
sudo pacman -S qt6-base qt6-svg opencv openvino eigen cmake gcc
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

### Debian/Ubuntu
```bash
# Install dependencies
sudo apt-get install qt6-base-dev qt6-base-dev-tools libqt6svg6-dev \
  libopencv-dev libopenvino-dev libeigen3-dev cmake build-essential

# Clone and build
git clone https://github.com/mybna134/ATLabelMaster.git
cd ATLabelMaster
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Install
sudo make install
```

### Fedora/RHEL
```bash
# Install dependencies
sudo dnf install qt6-qtbase-devel qt6-qtsvg-devel opencv-devel \
  openvino-devel eigen3-devel cmake gcc-c++

# Clone and build
git clone https://github.com/mybna134/ATLabelMaster.git
cd ATLabelMaster
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Install
sudo make install
```

### Generic Installer
```bash
git clone https://github.com/mybna134/ATLabelMaster.git
cd ATLabelMaster
sudo ./packaging/install.sh
```

---

## Usage

### Basic Workflow

1. **Open Folder** - Press `O` or click "打开文件夹" to select your image dataset
2. **Select Class** - Click on a class in the left panel (1-5, B, G, O)
3. **Annotate**
   - **Manual**: Click and drag to draw armor plate regions
   - **Smart**: Press `Space` for AI-assisted detection
4. **Navigate** - Use `Q`/`E` for previous/next image
5. **Save** - Press `S` or click "保存" to save labels
6. **Enhance** - Press `H` for histogram equalization

### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Open Folder | `O` |
| Save | `S` |
| Previous Image | `Q` |
| Next Image | `E` |
| Smart Annotate | `Space` |
| Histogram Equalize | `H` |
| Delete | `Delete` |
| Settings | `F1` |
| Select Class 1-5 | `1`-`5` |

### Settings

- **Dataset** - Configure save directories and paths
- **Behavior** - Toggle auto-save, auto-enhance
- **ROI** - Set fixed region of interest
- **Batch Replace** - Replace labels matching criteria
- **Interface** - Choose pixel art theme (requires restart)

---

## Themes

ATLabelMaster Pixel Art Edition includes three beautiful themes:

### Retro Gaming (Default)
- Inspired by 8-bit/16-bit game consoles
- Colors: Deep purple, cyan, coral red
- Pixel-perfect sharp corners
- Block-style UI elements

### Dark Modern
- VSCode-inspired dark theme
- Professional appearance
- Optimized for long sessions

### Classic
- Traditional Qt styling
- Familiar appearance
- Compatible with older workflows

To change themes, go to **Settings → Interface → Theme** and select your preferred theme. Restart the application for changes to take effect.

---

## Building from Source

### Prerequisites
- C++20 compiler (GCC 11+, Clang 13+)
- Qt6 (6.0+)
- OpenCV (4.0+)
- OpenVINO (2024.1+)
- Eigen3
- CMake (3.16+)

### Build Commands
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./bin/LabelMaster
```

### Building Packages
```bash
# All packages
./packaging/build_all.sh

# Individual packages
./packaging/arch/build.sh  # Arch Linux
./packaging/deb/build.sh   # Debian/Ubuntu
./packaging/rpm/build.sh   # Fedora/RHEL
```

---

## Project Structure

```
ATLabelMaster/
├── labelmaster/src/
│   ├── ui/                    # UI components
│   │   ├── pixel_widgets/     # Pixel-styled widgets
│   │   │   ├── theme_manager.*      # Theme system
│   │   │   ├── pixel_button.*       # Pixel buttons
│   │   │   ├── pixel_slider.*       # Pixel sliders
│   │   │   ├── pixel_checkbox.*     # Pixel checkboxes
│   │   │   ├── pixel_dialog.*       # Pixel dialog base
│   │   │   └── pixel_canvas.*       # Pixel canvas
│   │   ├── mainwindow.*       # Main window
│   │   ├── settings_dialog.*  # Settings dialog
│   │   ├── image_canvas.*     # Image canvas
│   │   └── info_dialog.*      # Info dialog
│   ├── controller/            # Settings management
│   ├── detector/              # AI detection
│   ├── service/               # File operations
│   ├── dataset/               # Dataset handling
│   ├── util/                  # Utilities
│   │   ├── image_cache.*      # LRU image cache
│   │   ├── crash_handler.*    # Crash recovery
│   │   └── keyboard_shortcuts.* # Keyboard manager
│   └── logger/                # Logging system
├── assets/
│   ├── themes/                # Theme JSON files
│   │   ├── retro.json         # Retro Gaming theme
│   │   ├── dark.json          # Dark Modern theme
│   │   └── classic.json       # Classic theme
│   ├── icons/                 # SVG icons
│   ├── models/                # ML models
│   └── label/                 # Label definitions
├── packaging/
│   ├── arch/PKGBUILD          # Arch Linux package
│   ├── deb/build.sh           # Debian builder
│   ├── rpm/labelmaster.spec   # RPM spec
│   ├── install.sh             # Universal installer
│   └── uninstall.sh           # Universal uninstaller
└── .github/workflows/         # CI/CD pipelines
```

---

## Architecture

### Design Patterns
- **Signal-Slot** (Qt) - Component communication
- **Singleton** - ThemeManager, ImageCache, CrashHandler
- **RAII** - AutoSaveGuard, ImageCacheGuard

### Key Classes
| Class | Purpose |
|-------|---------|
| `ThemeManager` | Manages pixel art themes |
| `ImageCanvas` | Image display and annotation |
| `SmartDetector` | AI-based armor detection |
| `FileService` | File operations and navigation |
| `AppSettings` | Configuration persistence |

---

## Development

### Code Style
```bash
# Format code with clang-format
find labelmaster/src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Testing
```bash
# Run tests (coming soon)
cd build
ctest
```

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

---

## License

MIT License - see LICENSE file for details.

---

## Credits

- **Original Author**: mybna134
- **Pixel Art Edition**: Contributors welcome!
- **AI Detection**: OpenVINO toolkit
- **Build System**: CMake + Qt6

---

## Roadmap

- [ ] Undo/Redo support
- [ ] More pixel art themes
- [ ] Custom theme editor
- [ ] Plugin system
- [ ] Windows and macOS support
- [ ] Performance profiling
- [ ] Unit tests
- [ ] Localization (i18n)

---

<div align="center">

**Made with ❤️ and pixels**

[Report Issues](https://github.com/mybna134/ATLabelMaster/issues) •
[Request Feature](https://github.com/mybna134/ATLabelMaster/issues)

</div>
