# ATLabelMaster Architecture Documentation

## Overview

ATLabelMaster is a Qt-based application for annotating armor plates in RoboMaster competition images. The application follows a Model-View-Controller (MVC) pattern with signal/slot communication.

## Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│                        UI Layer                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ MainWindow   │  │ ImageCanvas  │  │  Settings    │    │
│  │  (Coordinator)│  │  (Viewer)    │  │   Dialog     │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ signals/slots
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     Service Layer                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ FileService  │  │SmartDetector │  │Application   │    │
│  │  (I/O Mgr)   │  │ (AI/CV)      │  │   Wiring     │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      Utility Layer                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │SvgConstants  │  │ColorMapper   │  │ ImageCache   │    │
│  │Result<T>     │  │IdConvert     │  │CrashHandler  │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## Component Overview

### UI Layer

#### MainWindow
- **Responsibility**: Application coordinator and main window
- **Key Signals**:
  - `sigOpenFolderRequested` - Open image folder dialog
  - `sigFileActivated(QModelIndex)` - User selected a file
  - `sigNextRequested` / `sigPrevRequested` - Navigation
  - `sigDeleteRequested` - Delete current file
- **Key Slots**:
  - `setFileModel(QAbstractItemModel*)` - Set file tree model
  - `showImage(const QImage&)` - Display image in canvas
  - `setStatus(const QString&, int)` - Show status message

#### ImageCanvas
- **Responsibility**: Image display and annotation editing
- **Sub-components** (after refactoring):
  - `ImageRenderer` - Coordinate transforms, scaling, panning
  - `AnnotationManager` - Armor data CRUD
  - `InteractionHandler` - Mouse/keyboard event handling
  - `SvgOverlayRenderer` - SVG template rendering
  - `MaskManager` - Mask region management
- **Key Signals**:
  - `detectRequested(const QImage&)` - Request AI detection
  - `annotationCommitted(const Armor&)` - User finished creating annotation
  - `annotationsPublished(...)` - Request save annotations
- **Key Slots**:
  - `setDetections(const QVector<Armor>&)` - Load annotations
  - `histEqualize()` - Apply histogram equalization

### Service Layer

#### FileService
- **Responsibility**: File system operations and label I/O
- **Sub-components** (after refactoring):
  - `FileManager` - Directory navigation
  - `LabelIO` - Label file read/write
  - `DatasetFormatHandler` - Format abstraction
  - `BatchProcessor` - Batch operations
- **Key Signals**:
  - `modelReady(QAbstractItemModel*)` - File tree ready
  - `imageReady(const QImage&)` - Image loaded
  - `labelsLoaded(const QVector<Armor>&)` - Annotations loaded
  - `status(const QString&, int)` - Status message
- **Key Slots**:
  - `openFolderDialog(DataSet)` - Open folder
  - `openIndex(QModelIndex)` - Open specific file
  - `saveData(...)` - Save annotations

#### SmartDetector
- **Responsibility**: AI and traditional CV detection
- **Modes**:
  - `Traditional` - OpenCV-based detection
  - `AI` - OpenVINO YOLO model
- **Key Signals**:
  - `detected(const QVector<Armor>&)` - Detection results
  - `debugImages(...)` - Debug visualization
  - `error(const QString&)` - Detection error

#### ApplicationWiring
- **Responsibility**: Centralize signal/slot connections
- **Purpose**: Makes wiring explicit and testable
- **Methods**:
  - `wire()` - Establish all connections
  - `connectMainWindowToFileService()` - UI→Service connections
  - `connectFileServiceToMainWindow()` - Service→UI connections
  - etc.

### Utility Layer

#### SvgConstants
- **Purpose**: Centralized SVG template data
- **Data**:
  - Small armor: 557×516, anchors at specific positions
  - Big armor: 871×478, anchors at specific positions
- **Usage**: Perspective transformation for bbox calculation

#### ColorMapper
- **Purpose**: Color-to-class mapping
- **Mapping**:
  - 'R' → Red (255, 70, 70)
  - 'B' → Blue (61, 165, 255)
  - 'G' → Gray (170, 170, 180)
  - 'P' → Pink (255, 192, 203)

#### ImageCache
- **Purpose**: LRU cache for loaded images
- **Thread Safety**: Mutex-protected
- **Eviction**: LRU policy

#### CrashHandler
- **Purpose**: Crash recovery and logging
- **Mechanism**: Lock file + state JSON
- **Thread Safety**: `std::call_once` for singleton

## Signal/Slot Flow Diagrams

### Opening an Image Folder

```
User → MainWindow.sigOpenFolderRequested
         ↓
       FileService.openFolderDialog()
         ↓
       FileService.openDir()
         ↓
       FileService.modelReady(QAbstractItemModel*)
         ↓
       MainWindow.setFileModel()
         ↓
       FileService.tryOpenFirstAfterLoaded()
         ↓
       FileService.imageReady(QImage)
         ↓
       MainWindow.showImage()
         ↓
       ImageCanvas.setImage()
```

### Creating an Annotation

```
User (drag on canvas)
         ↓
    ImageCanvas.mousePressEvent()
         ↓
    ImageCanvas.mouseMoveEvent() [update drag]
         ↓
    ImageCanvas.mouseReleaseEvent()
         ↓
    ImageCanvas.annotationCommitted(Armor)
         ↓
    ImageCanvas.annotationsPublished(armors, image, needSaveImg)
         ↓
    FileService.saveData()
         ↓
    LabelIO.writeLabelFile()
         ↓
    FileService.status("已保存标注")
```

### AI Detection Flow

```
User clicks "Smart Detect"
         ↓
    ImageCanvas.detectRequested(QImage)
         ↓
    SmartDetector.detect()
         ↓
    [AI Model / OpenCV Detection]
         ↓
    SmartDetector.detected(QVector<Armor>)
         ↓
    ImageCanvas.setDetections()
         ↓
    [Update display]
```

## Data Format

### Label File Format

#### LabelMaster2 Format (11 fields)
```
color size cls x0 y0 x1 y1 x2 y2 x3 y3
```
- All coordinates normalized to [0, 1]
- `x0,y0,x1,y1,x2,y2,x3,y3`: Four corner points (TL, BL, BR, TR)

#### LabelMasterV4 Format (13 fields)
```
cls x_c y_c w h x0 y0 x1 y1 x2 y2 x3 y3
```
- `cls`: Class token (e.g., "R1", "B2", "G3") - color * size * num
- `x_c,y_c,w,h`: Normalized bounding box (center format) [0,1]
- `x0,y0,x1,y1,x2,y2,x3,y3`: Normalized four corner points [0,1]
- Size is determined from bbox aspect ratio (threshold: 2.5)

#### Extended Format (15 fields)
```
color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3
```
- Adds bounding box: `x,y,w,h` (center x, center y, width, height)
- Coordinates normalized to [0, 1]

### Armor Structure
```cpp
struct Armor {
    QString cls;      // Class token (e.g., "R1", "B2", "G3")
    QString color;    // Color letter (R/B/G/P)
    int size;         // 0=small, 1=big
    QPointF p0, p1, p2, p3;  // Corner points (TL, BL, BR, TR)
    double norm_x, norm_y, norm_w, norm_h;  // Normalized bbox (optional)
};
```

## Coordinate Systems

### Image Coordinates
- Origin: Top-left of image
- Units: Pixels
- Used for: Storage, calculations

### Widget Coordinates
- Origin: Top-left of ImageCanvas widget
- Units: Pixels
- Used for: Rendering, mouse input

### Normalized Coordinates
- Range: [0, 1]
- Used for: Label file storage
- Transform: `normalized = image_pixel / image_size`

### SVG Coordinates
- Origin: Top-left of SVG template
- Used for: Perspective transformation

## Key Algorithms

### Perspective Transformation (SVG → Image)

1. Get SVG template (small or big armor)
2. Get SVG anchor points
3. Get image anchor points (from user annotations)
4. Compute `QTransform::quadToQuad(svg_anchors, img_anchors)`
5. Transform SVG bounding box to image space

### BBox Calculation from Four Points

1. Transform SVG corners using perspective transform
2. Find min/max of transformed corners
3. Calculate center and dimensions

### Point-in-Polygon Test

```cpp
bool pointInsidePoly(const QPolygonF& poly, const QPointF& point) {
    return poly.containsPoint(point, Qt::OddEvenFill);
}
```

## Thread Safety

### ImageCache
- Protected by `QMutex mutex_`
- All public methods use `QMutexLocker`

### CrashHandler
- Singleton using `std::call_once`
- Signal handler is async-signal-safe

### FileService
- Operations are sequential (single-threaded UI context)
- No explicit mutex needed

## Build System

### CMake Structure
```
CMakeLists.txt
├── GLOB_RECURSE for all .cpp/.hpp/.ui files
├── Qt6::Widgets, Core, Gui, Svg
├── OpenCV, OpenVINO, Eigen3
└── AUTOMOC, AUTOUIC enabled
```

### Adding New Files
Due to `GLOB_RECURSE`, new files are automatically picked up after:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

## Refactoring Progress

### Phase 1: Bug Fixes ✅
- Fixed "BIg" typo
- Fixed file handle leaks
- Fixed CrashHandler race condition
- Simplified signal handler

### Phase 2: Code Quality ✅
- Created `SvgConstants` class
- Created `ColorMapper` class
- Created `IDetector` interface
- Created `Result<T>` error handling

### Phase 3: UX Improvements ✅
- Increased status message duration to 3000ms
- Verified dialog lifecycle management

### Phase 4: Architecture Refactoring ✅
- Created `ApplicationWiring` class ✅
- Created ImageCanvas sub-components ✅
- Created `DatasetFormatHandler` hierarchy ✅
- Created `LabelIO` class (partial)

### Phase 5: Documentation & Testing (In Progress)
- Architecture documentation ✅ (this file)
- Unit tests
- Integration tests

### Recent Additions
- Added `LabelMasterV4` format support (13 fields: cls x_c y_c w h pts[4,2]) ✅

## Future Work

1. Complete ImageCanvas refactoring (integrate sub-components)
2. Complete FileService refactoring (integrate LabelIO)
3. Add mode indicator for modifier keys (Alt/Shift/Ctrl)
4. Implement undo/redo with Command pattern
5. Add comprehensive unit tests
6. Add integration tests
7. Performance profiling
8. Memory leak detection with valgrind
