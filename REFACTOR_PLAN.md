# ATLabelMaster Refactoring Plan
## Generated: 2026-01-28
## Branch: refactor-planning-20260128

---

## Executive Summary

This refactoring plan addresses **37 critical/high-priority issues** and **28 medium/low-priority issues** identified through comprehensive code analysis. The plan is organized into **4 phases** that can be executed progressively.

### Priority Distribution
- **Critical** (data loss/crash): 12 issues
- **High** (productivity/maintainability): 18 issues
- **Medium** (UX/quality): 21 issues
- **Low** (style/cleanup): 14 issues

---

## Phase 1: Critical Bug Fixes (1-2 weeks)

### 1.1 File I/O Safety Issues

#### 1.1.1 Fix Resource Leaks in File Operations
**Files**: `file.cpp:228-239`, `file.cpp:396-556`
**Priority**: CRITICAL
**Issues**:
- QBuffer in `tryImportDataSetAfterLoaded()` not closed on all paths
- QFile in `openFileAt()` only closed on success path
- No RAII pattern for resource management

**Solution**:
```cpp
// Use RAII wrapper for file operations
class FileGuard {
    QFile& file_;
public:
    FileGuard(QFile& f) : file_(f) { file_.open(...); }
    ~FileGuard() { if (file_.isOpen()) file_.close(); }
};

// Usage in openFileAt()
QFile labelFile(lbl);
FileGuard guard(labelFile);  // Auto-closes on scope exit
```

#### 1.1.2 Fix Division by Zero in Image Canvas
**File**: `image_canvas.cpp:845-875`
**Priority**: CRITICAL
**Issue**: Parallel mode divides by vector length without checking for zero
**Solution**:
```cpp
case 0: {
    const QPointF t = A.p3 - A.p2;
    if (std::abs(t.y()) < 0.001) {  // Add check
        // Handle horizontal line case
        return; // or use alternative calculation
    }
    tx = A.p1.x() + (t.x() / t.y()) * (A.p0.y() - A.p1.y());
    break;
}
```

#### 1.1.3 Add Bounds Checking in Array Access
**File**: `file.cpp:882-884`
**Priority**: CRITICAL
**Issue**: `t.at(i)` called without bounds validation
**Solution**:
```cpp
auto tod = [&](int i) -> double {
    if (i >= t.size()) return 0.0;  // Add bounds check
    bool o = false;
    double v = t[i].toDouble(&o);
    ok &= o;
    return v;
};
```

### 1.2 State Management Fixes

#### 1.2.1 Fix Uninitialized currentClass_
**File**: `mainwindow.hpp:87`
**Priority**: CRITICAL
**Solution**:
```cpp
QString currentClass_ = QStringLiteral("G");  // Set default class
```

#### 1.2.2 Fix Double Signal Connection
**File**: `mainwindow.cpp:42-43`
**Priority**: HIGH
**Issue**: Both `activated` and `doubleClicked` connected to same slot
**Solution**: Remove the `doubleClicked` connection (Qt already emits `activated` on double-click)

#### 1.2.3 Fix State Corruption in Mask Mode
**File**: `image_canvas.cpp:685-718`
**Priority**: HIGH
**Issue**: `isMaskMode` flag can be left in inconsistent state
**Solution**: Use proper state machine or RAII pattern for mode tracking

---

## Phase 2: UX & User Experience Improvements (2-3 weeks)

### 2.1 Keyboard & Navigation Improvements

#### 2.1.1 Replace Non-Standard Navigation Shortcuts
**Files**: `mainwindow.cpp:205-212`, `mainwindow.ui`
**Current**: Q (prev), E (next)
**Problem**: Highly non-intuitive, conflicts with standard conventions
**Proposed**:
- **Option A**: Left/Right arrows (standard)
- **Option B**: A/D (gaming convention)
- **Option C**: Ctrl+Left/Ctrl+Right (many tools)

Let users choose in settings.

#### 2.1.2 Add Keyboard Shortcut Reference
**File**: Create new `shortcuts_dialog.ui/.cpp/.hpp`
**Features**:
- Display all keyboard shortcuts in organized table
- Group by function (Navigation, Annotation, File Operations)
- Allow customization (future phase)
- Add F1 or ? menu item to open dialog

#### 2.1.3 Add Missing Canvas Operations Shortcuts
**File**: `mainwindow.hpp`, `mainwindow.cpp`
**Add**:
```
Delete          - Delete selected annotation (vs current file)
Ctrl+Z          - Undo last annotation
Ctrl+Y          - Redo
Ctrl+D          - Duplicate selected annotation
+/-             - Zoom in/out
Space+drag      - Pan canvas
1-9             - Select class (already works, add visual feedback)
```

### 2.2 Visual Feedback Improvements

#### 2.2.1 Add Unsaved Changes Indicator
**File**: `mainwindow.cpp`, `mainwindow.hpp`
**Implementation**:
```cpp
class MainWindow {
    bool hasUnsavedChanges_ = false;

    void setUnsavedChanges(bool unsaved) {
        hasUnsavedChanges_ = unsaved;
        setWindowTitle(unsaved ? "ATLabelMaster *" : "ATLabelMaster");
        // Update UI indicator
    }

    void closeEvent(QCloseEvent* e) override {
        if (hasUnsavedChanges_) {
            auto reply = QMessageBox::question(
                this, "未保存的更改",
                "有未保存的标注，是否保存?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            // Handle...
        }
    }
};
```

#### 2.2.2 Add Completion Feedback for Smart Annotation
**File**: `mainwindow.cpp:62`, `image_canvas.cpp`
**Implementation**:
```cpp
// Add signal to ImageCanvas
signals:
    void detectionCompleted(int count, float timeMs);

// Connect in MainWindow
connect(ui_->label, &ImageCanvas::detectionCompleted, this,
    [this](int count, float time) {
        QString msg = tr("检测完成: 找到 %1 个装甲 (用时 %2ms)").arg(count).arg(time);
        setStatus(msg, 3000);
    });
```

#### 2.2.3 Add Mode Indicators for Constraint Modes
**File**: `image_canvas.cpp:845-887`
**Implementation**:
- Change cursor when Alt (parallel) or Shift (parallelogram) is pressed
- Show status message: "平行约束模式" or "平行四边形模式"
- Add visual indicator in canvas (overlay text or icon)

### 2.3 Workflow Improvements

#### 2.3.1 Add Undo/Redo Stack
**File**: Create `undo_stack.hpp/.cpp`
**Interface**:
```cpp
class UndoStack {
    struct Action {
        enum Type { AddAnnotation, RemoveAnnotation, ModifyAnnotation };
        Type type;
        Armor before;  // State before action
        Armor after;   // State after action
    };
    QVector<Action> stack_;
    int currentPos_ = -1;

public:
    void push(const Action& action);
    bool canUndo() const;
    bool canRedo() const;
    Action undo();
    Action redo();
};
```

#### 2.3.2 Add Copy Annotations from Previous Image
**File**: `mainwindow.cpp`, add menu item and shortcut
**Implementation**: Copy all annotations from current image to clipboard, paste to next image

#### 2.3.3 Add Batch Class Selection
**File**: `mainwindow.cpp`
**Feature**: Select multiple annotations (Ctrl+click), change class for all at once

---

## Phase 3: Architecture & Code Quality (3-4 weeks)

### 3.1 Refactor God Classes

#### 3.1.1 Extract Keyboard Handling
**Files**: `mainwindow.cpp:180-248`, create `keyboard_handler.hpp/.cpp`
**Responsibility**: Handle all keyboard shortcuts and input
**Interface**:
```cpp
class KeyboardHandler : public QObject {
    Q_OBJECT
public:
    KeyboardHandler(QMainWindow* parent);
    void setActionRegistry(ActionRegistry* registry);

signals:
    void navigationRequested(NavigationDirection);
    void annotationRequested(AnnotationAction);
    void toolRequested(ToolType);

public slots:
    void handleKeyPress(QKeyEvent* e);
    bool keyPressHasFocus() const;
};
```

#### 3.1.2 Extract Dialog Management
**Files**: `mainwindow.cpp:71-81`, create `dialog_manager.hpp/.cpp`
**Responsibility**: Dialog lifecycle, prevent multiple instances
**Interface**:
```cpp
class DialogManager {
    template<typename T>
    T* showOrCreateDialog() {
        static T* instance = nullptr;
        if (!instance) {
            instance = new T(parent_);
            instance->setAttribute(Qt::WA_DeleteOnClose);
        }
        instance->show();
        instance->raise();
        instance->activateWindow();
        return instance;
    }
};
```

#### 3.1.3 Split Long Functions
**File**: `file.cpp:382-565` (183 lines)
**Current**: `tryImportDataSetAfterLoaded()`
**Refactor into**:
- `tryImportDataSetAfterLoaded()` - orchestrate
- `importLabelMasterV1(QFile&, const QString&)`
- `importLabelMasterV3(QFile&, const QString&)`
- `importHITSZ(QFile&, const QString&)`
- `importUPC(QFile&, const QString&)`
- `importNWPU(QFile&, const QString&)`

**File**: `file.cpp:728-855` (127 lines)
**Current**: `writeLabelFile()`
**Refactor into**:
- `writeLabelFile()` - orchestrate
- `computeBoundingBox(const Armor&, const QSize&)`
- `transformSVGToImage(const Armor&, const QSize&)`
- `writeArmorRecord(QTextStream&, const Armor&, const QSize&)`

**File**: `settings_dialog.cpp:183-381` (198 lines)
**Current**: `performBatchReplace()`
**Refactor into**:
- `performBatchReplace()` - orchestrate
- `findLabelFiles(const QDir&)` -> QStringList
- `parseLabelFile(const QString&)` -> QVector<Armor>
- `applyReplacements(QVector<Armor>&, const ReplacementRule&)`
- `generateReport(const BatchStats&)` -> QString

### 3.2 Extract & Standardize Utilities

#### 3.2.1 Create Format Conversion Utilities
**File**: Create `label_format.hpp/.cpp`
**Purpose**: Centralize label format handling
```cpp
namespace LabelFormat {
    enum class Type {
        Unknown,
        PointsOnly,      // 11 fields
        RectWithPoints   // 15 fields
    };

    Type detectFormat(const QStringList& fields);
    QString formatToString(Type type);
    QStringList getFormatFields(Type type);
    bool validateFormat(const QStringList& fields, Type expected);

    // Format conversion
    struct Record {
        int colorId;
        int size;
        int classId;
        QVector<QPointF> points;
        QRectF bbox;  // Optional for RectWithPoints
    };

    Record parseRecord(const QString& line, Type type);
    QString formatRecord(const Record& rec, Type outputType);
}
```

#### 3.2.2 Create File I/O Utilities
**File**: create `file_utils.hpp/.cpp`
**Purpose**: RAII wrappers and helpers for file operations
```cpp
namespace FileUtils {
    // RAII file handle
    class FileGuard {
        QFile& file_;
    public:
        FileGuard(QFile& f, QIODevice::OpenMode mode);
        ~FileGuard();
        bool isOpen() const;
    };

    // UTF-8 file reading/writing
    struct TextFile {
        static QStringList readUtf8(const QString& path);
        static bool writeUtf8(const QString& path, const QStringList& lines);
        static QString readAllUtf8(const QString& path);
    };

    // Path utilities
    QString canonicalPath(const QString& path);
    QString labelPathForImage(const QString& imagePath, const QString& labelDir);
    bool ensureDirExists(const QString& path);
}
```

#### 3.2.3 Create SVG Utilities
**File**: create `svg_utils.hpp/.cpp`
**Purpose**: Centralize SVG-related calculations
```cpp
namespace SVGUtils {
    struct ArmorTemplate {
        QSize size;
        QVector<QPointF> anchors;  // Normalized [0,1]
        QString iconName;
    };

    ArmorTemplate getTemplate(bool isBigArmor);
    QRectF computeBoundingBox(const QVector<QPointF>& anchors,
                               const QSize& imageSize,
                               const ArmorTemplate& tmpl);
    QVector<QPointF> transformTemplate(const QVector<QPointF>& imagePoints,
                                       const ArmorTemplate& tmpl);
}
```

### 3.3 Add Resource Management

#### 3.3.1 Fix SVG Cache Memory Leak
**File**: `image_canvas.cpp:1114-1138`
**Solution**:
```cpp
class ImageCanvas {
private:
    struct SvgCache {
        QHash<int, QHash<int, QSvgRenderer*>> cache;
        ~SvgCache() {
            for (auto& row : cache) {
                for (auto* renderer : row) {
                    delete renderer;
                }
            }
        }
        void clear() {
            for (auto& row : cache) {
                for (auto* renderer : row) {
                    delete renderer;
                }
            }
            cache.clear();
        }
    };
    SvgCache svgCache_;
};
```

#### 3.3.2 Add Smart Pointer Usage
**Files**: Multiple files
**Action**: Gradually migrate to smart pointers where ownership is clear
```cpp
// Instead of:
QAction* action = new QAction(this);

// Use:
std::unique_ptr<QAction> action = std::make_unique<QAction>(this);
// Or where Qt takes ownership:
auto* action = new QAction(this);  // Qt parent takes ownership
```

### 3.4 Fix Duplicate Code Patterns

#### 3.4.1 Consolidate Transform Logic
**Files**: `image_canvas.cpp:286-293`, `462-469`
**Solution**: Move to `svg_utils.hpp/.cpp` (see 3.2.3)

#### 3.4.2 Consolidate Qt Version Checks
**Files**: Multiple files have `#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)`
**Solution**: Create `qt_compat.hpp`
```cpp
namespace QtCompat {
    inline void setUtf8Encoding(QTextStream& stream) {
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            stream.setEncoding(QStringConverter::Utf8);
        #else
            stream.setCodec("UTF-8");
        #endif
    }
}
```

---

## Phase 4: Progressive Enhancements (2-3 weeks)

### 4.1 Add Configuration Validation

#### 4.1.1 Add Input Validation for Settings
**File**: `settings_dialog.cpp`
**Add**: Validators for all text fields
```cpp
void SettingsDialog::setSaveDir() {
    QString newDir = this->ui_->dataset_dir_edit->text();

    // Validate not empty
    if (newDir.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("保存目录不能为空"));
        this->ui_->dataset_dir_edit->setText(
            controller::AppSettings::instance().saveDir());
        return;
    }

    // Validate directory exists
    QDir dir(newDir);
    if (!dir.exists()) {
        auto reply = QMessageBox::question(
            this, tr("目录不存在"),
            tr("目录不存在: %1\n是否创建?").arg(newDir),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (!dir.mkpath(newDir)) {
                QMessageBox::critical(this, tr("错误"),
                    tr("无法创建目录: %1").arg(newDir));
                return;
            }
        } else {
            return;
        }
    }

    // Validate writable
    QFile testFile(newDir + "/.write_test");
    if (!testFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("错误"), tr("目录不可写"));
        return;
    }
    testFile.remove();

    controller::AppSettings::instance().setsaveDir(newDir);
    update();
}
```

#### 4.1.2 Fix Inconsistent Default Values
**File**: `settings.hpp:104-115`
**Changes**:
```cpp
// Fix ROI default inconsistency
static constexpr const int kRoiW  = 640;  // Match UI file
static constexpr const int kRoiH  = 480;

// Fix assetsDir default - use relative path
static constexpr const char* kAssetsDir = "assets";  // Relative to app dir

// Fix vRate default - too high for default
static constexpr const float kVRate = 1.0f;  // More reasonable default
```

### 4.2 Improve Error Messages & Logging

#### 4.2.1 Add Silent Fail Warnings
**File**: `file.cpp:857-977`
**Current**: Malformed lines are silently skipped
**Fix**:
```cpp
int skippedLines = 0;
QVector<int> skippedLineNumbers;
while (!ts.atEnd()) {
    QString line = ts.readLine();
    int lineNum++;
    const QStringList t = line.simplified().split(' ');
    if (t.size() != 11 && t.size() != 15) {
        skippedLines++;
        skippedLineNumbers.append(lineNum);
        continue;
    }
    // ...
}
if (skippedLines > 0) {
    LOGW(QString("跳过 %1 行格式错误的标签 (行: %2)")
        .arg(skippedLines)
        .arg(QStringList::number(skippedLineNumbers.mid(0, 5).join(", ")) +
            (skippedLineNumbers.size() > 5 ? "..." : "")));
}
```

#### 4.2.2 Standardize Error Message Format
**Files**: Multiple locations
**Standard**:
```
- User-facing: Use tr() with clear Chinese
- Logging: Use LOGE/W/I() with context
- Debug: Use qDebug() with file:line prefix
```

### 4.3 Add Performance Optimizations

#### 4.3.1 Implement Paint Caching for Annotations
**File**: `image_canvas.cpp:408-563`
**Current**: Recalculates SVG transforms on every paint
**Solution**:
```cpp
class ImageCanvas {
private:
    struct CacheEntry {
        QPixmap polygon;
        QPixmap text;
        bool valid = false;
    };
    QHash<int, CacheEntry> renderCache_;

    void invalidateCache(int index = -1) {
        if (index == -1) {
            for (auto& entry : renderCache_) {
                entry.valid = false;
            }
        } else {
            renderCache_[index].valid = false;
        }
        update();
    }

    void drawDetections(QPainter& p) override {
        for (int i = 0; i < dets_.size(); ++i) {
            if (!renderCache_[i].valid) {
                // Render to cache
                renderCache_[i].polygon = renderPolygonToPixmap(dets_[i]);
                renderCache_[i].text = renderTextToPixmap(dets_[i]);
                renderCache_[i].valid = true;
            }
            // Draw from cache
            p.drawPixmap(pos, renderCache_[i].polygon);
        }
    }
};
```

#### 4.3.2 Optimize Label File Reading
**File**: `file.cpp:1031-1079` (getStas function)
**Current**: Opens each file individually, re-parses every line
**Solution**: Cache parsed data
```cpp
class FileService {
private:
    struct CacheKey {
        QString filePath;
        qint64 modifiedTime;
    };
    QHash<CacheKey, QVector<Armor>> labelCache_;

    QVector<Armor> readLabelFileCached(const QString& path) {
        QFileInfo fi(path);
        CacheKey key{path, fi.lastModified().toMSecsSinceEpoch()};

        if (labelCache_.contains(key)) {
            return labelCache_[key];
        }

        auto armors = readLabelFile(path, currentImageSize_);
        labelCache_[key] = armors;
        return armors;
    }
};
```

### 4.4 Add Documentation & Help

#### 4.4.1 Add Tooltips Throughout Settings Dialog
**File**: `settings_dialog.ui`
**Add tooltips for**:
- ROI section: "锁定检测区域大小，避免每次重新计算ROI"
- Label format: "选择输出格式。11字段:仅关键点; 15字段:包含边界框"
- Batch replace: "提示：选择 'All' 可匹配所有颜色/大小"
- vRate slider: "图像亮度增强倍数 (1.0-10.0)"

#### 4.4.2 Add User Guide
**File**: Create `docs/user_guide.md`
**Sections**:
1. Quick Start (5-minute tutorial)
2. Keyboard Shortcuts Reference
3. Dataset Import Guide
4. Label Format Specifications
5. Tips & Tricks

#### 4.4.3 Add Inline Code Documentation
**Files**: All major class files
**Action**: Add doxygen comments for all public APIs
```cpp
/**
 * @brief Manages file I/O operations for image and label files.
 *
 * The FileService handles:
 * - Directory navigation and file filtering
 * - Image loading and caching
 * - Label file reading/writing
 * - Dataset import from various formats
 *
 * @note All file paths are normalized to absolute paths
 * @warning Not thread-safe - use from main thread only
 */
class FileService : public QObject {
    // ...
};
```

---

## Progressive Refactoring Roadmap

### Week 1-2: Critical Fixes
- [ ] Fix resource leaks in file operations
- [ ] Fix division by zero in image canvas
- [ ] Add bounds checking in array access
- [ ] Fix double signal connection
- [ ] Initialize currentClass_
- [ ] Fix state corruption in mask mode

### Week 3-4: UX Improvements
- [ ] Replace Q/E navigation with user-choice shortcuts
- [ ] Add keyboard shortcuts dialog
- [ ] Add canvas operation shortcuts
- [ ] Implement unsaved changes indicator
- [ ] Add smart annotation completion feedback
- [ ] Add mode indicators for constraints

### Week 5-6: Architecture (Phase 1)
- [ ] Extract KeyboardHandler class
- [ ] Extract DialogManager class
- [ ] Split tryImportDataSetAfterLoaded()
- [ ] Split writeLabelFile()
- [ ] Create FileUtils utilities

### Week 7-8: Architecture (Phase 2)
- [ ] Create LabelFormat utilities
- [ ] Create SvgUtils
- [ ] Fix SVG cache memory leak
- [ ] Consolidate transform logic
- [ ] Add QtCompat utilities

### Week 9-10: Enhancements
- [ ] Add input validation for settings
- [ ] Fix inconsistent default values
- [ ] Add silent fail warnings
- [ ] Implement paint caching
- [ ] Add documentation and help

### Week 11+: Future Enhancements
- [ ] Undo/Redo stack implementation
- [ ] Batch class selection
- [ ] Copy annotations from previous image
- [ ] Shortcut customization system
- [ ] Comprehensive unit tests

---

## Testing Strategy

### Unit Tests
Create `tests/` directory with:
- `test_file_io.cpp` - FileService tests
- `test_label_format.cpp` - Format conversion tests
- `test_svg_utils.cpp` - SVG transform tests
- `test_keyboard_handler.cpp` - Keyboard handling tests

### Integration Tests
- `test_import_workflow.cpp` - Test all import formats
- `test_annotation_workflow.cpp` - Test full annotation workflow
- `test_batch_operations.cpp` - Test batch replace/convert

### UI Tests
- Manual test checklist for each refactored component
- Screenshot-based regression tests for critical UI paths

---

## Risk Assessment

### High Risk Areas
1. **File I/O refactoring** - Core functionality, data loss risk
   - Mitigation: Comprehensive tests, gradual migration

2. **Keyboard handling extraction** - UX regression risk
   - Mitigation: Keep old code paths during transition, extensive testing

3. **Signal/slot reorganization** - Connection breakage risk
   - Mitigation: Runtime connection verification, connection logging

### Medium Risk Areas
1. **Paint caching** - Visual regression risk
   - Mitigation: Cache invalidation verification, side-by-side comparison

2. **Settings validation** - User workflow disruption
   - Mitigation: Allow override via environment variable, clear error messages

---

## Success Criteria

Each phase is considered complete when:
1. All critical bugs in that phase are fixed
2. Unit tests pass for refactored code
3. Integration tests pass
4. Manual testing confirms no regression
5. Code review approves changes

---

## Next Steps

1. **Review and approve this plan** with stakeholders
2. **Create feature branch** from refactor-planning-20260128
3. **Begin Phase 1** - Critical bug fixes
4. **Set up CI/CD** to run tests on each commit
5. **Create tracking ticket** for each issue (GitHub Issues or JIRA)

---

## Appendix: Detailed Issue Lists

### Critical Issues (Must Fix)
| ID | File | Lines | Issue | Fix Effort |
|----|------|-------|-------|------------|
| C1 | file.cpp | 228-239 | Resource leak in openFileAt | 2h |
| C2 | file.cpp | 396-556 | Resource leak in import | 4h |
| C3 | image_canvas.cpp | 845-875 | Division by zero | 2h |
| C4 | file.cpp | 882-884 | Bounds check missing | 1h |
| C5 | mainwindow.hpp | 87 | Uninitialized currentClass_ | 0.5h |
| C6 | mainwindow.cpp | 42-43 | Double signal connection | 0.5h |
| C7 | image_canvas.cpp | 685-718 | Mask mode state corruption | 3h |
| C8 | file.cpp | 108-113 | Race condition in model reset | 4h |
| C9 | file.cpp | 728-855 | Missing error handling | 2h |
| C10 | file.cpp | 857-977 | Silent fail on parse error | 1h |
| C11 | file.cpp | 698-726 | Fragile path handling | 2h |
| C12 | file.cpp | 258-305 | Inconsistent autoSave | 1h |

### High Priority Issues
| ID | File | Lines | Issue | Fix Effort |
|----|------|-------|-------|------------|
| H1 | mainwindow.cpp | 205-212 | Non-intuitive Q/E nav | 2h |
| H2 | mainwindow.cpp | - | No unsaved indicator | 6h |
| H3 | mainwindow.cpp | - | No smart annotation feedback | 3h |
| H4 | image_canvas.cpp | 408-563 | Paint performance (no cache) | 8h |
| H5 | image_canvas.cpp | 1114-1138 | SVG memory leak | 3h |
| H6 | image_canvas.cpp | 225-243 | No bounds checking | 2h |
| H7 | image_canvas.cpp | 276-344, 459-499 | Duplicate transform logic | 4h |
| H8 | image_canvas.cpp | 152-181 | Complex state machine | 6h |
| H9 | image_canvas.cpp | 845-887 | No mode visual feedback | 3h |
| H10 | image_canvas.cpp | 722-740 | No undo for deletion | 4h |
| H11 | file.cpp | 382-565 | Function too long (import) | 6h |
| H12 | file.cpp | 728-855 | Function too long (write) | 4h |
| H13 | file.cpp | 1031-1079 | Inefficient statistics | 3h |
| H14 | file.cpp | - | Duplicate next/prev logic | 2h |
| H15 | settings_dialog.cpp | 183-381 | Function too long (batch) | 6h |
| H16 | settings_dialog.cpp | - | Duplicate format conversion | 3h |
| H17 | mainwindow.cpp | 180-248 | Complex keyPressEvent | 4h |
| H18 | mainwindow.cpp | - | God class antipattern | 12h |

---

*End of Refactoring Plan*
