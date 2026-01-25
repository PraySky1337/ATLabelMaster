/**
 * @file theme_manager.cpp
 * @brief Implementation of ThemeManager class
 *
 * FIXED: Now excludes image-display widgets from styling to prevent color distortion
 * FIXED: Added hot reload support with automatic widget updates
 */

#include "theme_manager.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QTimer>
#include <QDebug>
#include <QStyle>

namespace labelmaster::ui {

namespace {
    // Enhanced default theme colors
    const QHash<QString, QString> DEFAULT_COLORS = {
        // Background colors
        {"background", "#1a1b26"},           // Deep blue-black
        {"panel", "#24283b"},                // Lighter panel
        {"panel_alternate", "#1f2335"},      // Alternate panel
        {"text", "#c0caf5"},                 // Soft blue-white
        {"text_dim", "#565f89"},             // Dimmed text
        {"text_bright", "#ffffff"},          // Bright text

        // Border colors
        {"border", "#414868"},               // Subtle border
        {"border_light", "#565f89"},         // Light border
        {"border_dark", "#1a1b26"},          // Dark border

        // Accent colors - Enhanced palette
        {"accent_primary", "#7aa2f7"},       // Blue
        {"accent_secondary", "#bb9af7"},     // Purple
        {"accent_success", "#9ece6a"},       // Green
        {"accent_warning", "#e0af68"},       // Orange/Yellow
        {"accent_error", "#f7768e"},         // Red
        {"accent_info", "#7dcfff"},          // Cyan

        // Button states
        {"button_hover", "#8aafef"},
        {"button_pressed", "#6a8fd7"},
        {"button_disabled", "#414868"},

        // Input colors
        {"input_background", "#1a1b26"},
        {"input_text", "#c0caf5"},
        {"input_border", "#414868"},
        {"input_focus", "#7aa2f7"},

        // Selection
        {"selection", "#7aa2f7"},
        {"selection_text", "#1a1b26"},

        // Link
        {"link", "#7aa2f7"},
        {"link_visited", "#bb9af7"}
    };

    const QHash<QString, int> DEFAULT_DIMENSIONS = {
        {"button_border", 2},
        {"panel_border", 2},
        {"corner_radius", 0},      // Pixel style = no rounding
        {"spacing_small", 4},
        {"spacing_medium", 8},
        {"spacing_large", 16},
        {"icon_size", 16}
    };
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : QObject()
    , m_currentThemeId("retro_enhanced")
{
    // Determine assets path
    QStringList paths = {
        QDir::currentPath() + "/assets/themes",
        QDir::currentPath() + "/../assets/themes",
        QCoreApplication::applicationDirPath() + "/../assets/themes",
        "/usr/share/labelmaster/themes",
        "/usr/local/share/labelmaster/themes"
    };

    for (const QString& path : paths) {
        if (QDir(path).exists()) {
            m_assetsPath = path;
            break;
        }
    }

    if (m_assetsPath.isEmpty()) {
        m_assetsPath = QDir::currentPath() + "/assets/themes";
        QDir().mkpath(m_assetsPath);
    }

    discoverThemes();
    loadTheme("retro_enhanced");
}

void ThemeManager::discoverThemes() {
    QDir themesDir(m_assetsPath);
    if (!themesDir.exists()) {
        qWarning() << "Themes directory not found:" << m_assetsPath;
        return;
    }

    QStringList jsonFiles = themesDir.entryList(QStringList("*.json"), QDir::Files);
    for (const QString& fileName : jsonFiles) {
        QString filePath = themesDir.filePath(fileName);
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject themeObj = doc.object();
                QString themeId = themeObj.value("id").toString(fileName.chopped(5));
                m_availableThemes[themeId] = themeObj;
            }
            file.close();
        }
    }

    qDebug() << "Discovered themes:" << m_availableThemes.keys();
}

bool ThemeManager::loadTheme(const QString& themeId) {
    QJsonObject themeObj = loadThemeJson(themeId);
    if (themeObj.isEmpty()) {
        qWarning() << "Failed to load theme:" << themeId;
        return false;
    }

    m_currentTheme = themeObj;
    m_currentThemeId = themeId;

    qDebug() << "Loaded theme:" << themeId;

    // Apply theme immediately (hot reload)
    applyTheme();

    emit themeChanged(themeId);
    return true;
}

QJsonObject ThemeManager::loadThemeJson(const QString& themeId) const {
    if (m_availableThemes.contains(themeId)) {
        return m_availableThemes.value(themeId);
    }

    QString filePath = QDir(m_assetsPath).filePath(themeId + ".json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return QJsonObject();
    }

    return doc.object();
}

QString ThemeManager::currentThemeId() const {
    return m_currentThemeId;
}

QStringList ThemeManager::availableThemes() const {
    return m_availableThemes.keys();
}

QString ThemeManager::themeDisplayName(const QString& themeId) const {
    if (m_availableThemes.contains(themeId)) {
        return m_availableThemes.value(themeId).value("name").toString(themeId);
    }
    return themeId;
}

QColor ThemeManager::color(const QString& key) const {
    return color(key, QColor(0, 0, 0));
}

QColor ThemeManager::color(const QString& key, const QColor& fallback) const {
    QJsonObject colors = m_currentTheme.value("colors").toObject();
    QString colorStr = colors.value(key).toString();

    if (colorStr.isEmpty() && DEFAULT_COLORS.contains(key)) {
        colorStr = DEFAULT_COLORS.value(key);
    }

    if (colorStr.isEmpty()) {
        return fallback;
    }

    return QColor(colorStr);
}

QFont ThemeManager::uiFont() const {
    QJsonObject fonts = m_currentTheme.value("fonts").toObject();
    QString fontName = fonts.value("ui").toString("JetBrains Mono");

    QFont font(fontName);
    font.setPixelSize(12);
    return font;
}

QFont ThemeManager::monoFont() const {
    QJsonObject fonts = m_currentTheme.value("fonts").toObject();
    QString fontName = fonts.value("mono").toString("JetBrains Mono");

    QFont font(fontName);
    font.setPixelSize(11);
    return font;
}

int ThemeManager::dimension(const QString& key) const {
    return dimension(key, 0);
}

int ThemeManager::dimension(const QString& key, int fallback) const {
    QJsonObject dimensions = m_currentTheme.value("dimensions").toObject();
    if (dimensions.contains(key)) {
        return dimensions.value(key).toInt(fallback);
    }
    if (DEFAULT_DIMENSIONS.contains(key)) {
        return DEFAULT_DIMENSIONS.value(key);
    }
    return fallback;
}

bool ThemeManager::pixelScaling() const {
    QJsonObject effects = m_currentTheme.value("effects").toObject();
    return effects.value("pixel_scaling").toBool(true);
}

QString ThemeManager::animationSpeed() const {
    QJsonObject effects = m_currentTheme.value("effects").toObject();
    return effects.value("animation_speed").toString("medium");
}

QString ThemeManager::generateStyleSheet() const {
    QString css;

    // Common color shortcuts
    const QString bg = color("background").name();
    const QString panel = color("panel").name();
    const QString panelAlt = color("panel_alternate").name();
    const QString text = color("text").name();
    const QString textDim = color("text_dim").name();
    const QString border = color("border").name();
    const QString primary = color("accent_primary").name();
    const QString secondary = color("accent_secondary").name();
    const QString success = color("accent_success").name();
    const QString warning = color("accent_warning").name();
    const QString error = color("accent_error").name();
    const QString btnHover = color("button_hover").name();
    const QString btnPressed = color("button_pressed").name();
    const QString btnDisabled = color("button_disabled").name();

    const int btnBorder = dimension("button_border");
    const int panelBorder = dimension("panel_border");
    const int cornerRadius = dimension("corner_radius");

    // IMPORTANT: Exclude image-display classes to prevent color distortion
    // We use :not() selector to exclude ImageCanvas and similar widgets

    // Global application styles (EXCLUDING image widgets)
    css += "/* Global styles - excluding image widgets */\n";
    css += QString("QWidget:not(ImageCanvas):not(QLabel[image=true]):not(QLabel[pixmap=true]) {\n");
    css += QString("  font-family: %1;\n").arg(uiFont().family());
    css += QString("  font-size: %1pt;\n").arg(uiFont().pointSize());
    css += QString("  color: %1;\n").arg(text);
    css += QString("  background-color: %1;\n").arg(bg);
    css += "}\n\n";

    // QMainWindow
    css += "QMainWindow {\n";
    css += QString("  background-color: %1;\n").arg(bg);
    css += "}\n\n";

    // QDialog
    css += "QDialog {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  border: %1px solid %2;\n").arg(panelBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    // QPushButton - Pixel style
    css += "QPushButton {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += QString("  color: %1;\n").arg(bg);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 8px 16px;\n";
    css += "  font-weight: bold;\n";
    css += "}\n\n";

    css += "QPushButton:hover {\n";
    css += QString("  background-color: %1;\n").arg(btnHover);
    css += "}\n\n";

    css += "QPushButton:pressed {\n";
    css += QString("  background-color: %1;\n").arg(btnPressed);
    css += "  padding-top: 9px;\n";  // Press effect
    css += "  padding-left: 17px;\n";
    css += "}\n\n";

    css += "QPushButton:disabled {\n";
    css += QString("  background-color: %1;\n").arg(btnDisabled);
    css += QString("  color: %1;\n").arg(textDim);
    css += "}\n\n";

    // QFrame
    css += "QFrame {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  border: %1px solid %2;\n").arg(panelBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QFrame[frameShape=\"0\"], QFrame[frameShape=\"4\"] {\n";
    css += "  border: none;\n";
    css += "  background-color: transparent;\n";
    css += "}\n\n";

    // QLabel - Exclude image labels
    css += "QLabel:not([image=true]):not([pixmap=true]) {\n";
    css += QString("  color: %1;\n").arg(text);
    css += "  background-color: transparent;\n";
    css += "  border: none;\n";
    css += "}\n\n";

    // QLineEdit
    css += "QLineEdit {\n";
    css += QString("  background-color: %1;\n").arg(color("input_background").name());
    css += QString("  color: %1;\n").arg(color("input_text").name());
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(color("input_border").name());
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 6px;\n";
    css += "}\n\n";

    css += "QLineEdit:focus {\n";
    css += QString("  border: %1px solid %2;\n").arg(btnBorder + 1).arg(color("input_focus").name());
    css += "}\n\n";

    // QCheckBox
    css += "QCheckBox {\n";
    css += QString("  color: %1;\n").arg(text);
    css += "  spacing: 8px;\n";
    css += "}\n\n";

    css += "QCheckBox::indicator {\n";
    css += "  width: 18px;\n";
    css += "  height: 18px;\n";
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += QString("  background-color: %1;\n").arg(bg);
    css += "}\n\n";

    css += "QCheckBox::indicator:checked {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += "}\n\n";

    css += "QCheckBox::indicator:hover {\n";
    css += QString("  border: %1px solid %2;\n").arg(btnBorder + 1).arg(primary);
    css += "}\n\n";

    // QRadioButton
    css += "QRadioButton {\n";
    css += QString("  color: %1;\n").arg(text);
    css += "  spacing: 8px;\n";
    css += "}\n\n";

    css += "QRadioButton::indicator {\n";
    css += "  width: 18px;\n";
    css += "  height: 18px;\n";
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += QString("  background-color: %1;\n").arg(bg);
    css += "}\n\n";

    css += "QRadioButton::indicator:checked {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += "}\n\n";

    // QComboBox
    css += "QComboBox {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 6px 12px;\n";
    css += "}\n\n";

    css += "QComboBox:hover {\n";
    css += QString("  border: %1px solid %2;\n").arg(btnBorder + 1).arg(primary);
    css += "}\n\n";

    css += "QComboBox::drop-down {\n";
    css += "  border: none;\n";
    css += "  width: 24px;\n";
    css += "}\n\n";

    css += "QComboBox QAbstractItemView {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  selection-background-color: %1;\n").arg(primary);
    css += QString("  selection-color: %1;\n").arg(bg);
    css += "}\n\n";

    // QSlider
    css += "QSlider::groove:horizontal {\n";
    css += QString("  background-color: %1;\n").arg(border);
    css += "  height: 6px;\n";
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QSlider::handle:horizontal {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += "  width: 18px;\n";
    css += "  height: 18px;\n";
    css += QString("  margin: -6px 0;\n");
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QSlider::handle:horizontal:hover {\n";
    css += QString("  background-color: %1;\n").arg(btnHover);
    css += "}\n\n";

    // QSpinBox
    css += "QSpinBox {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 6px;\n";
    css += "}\n\n";

    css += "QSpinBox:focus {\n";
    css += QString("  border: %1px solid %2;\n").arg(btnBorder + 1).arg(primary);
    css += "}\n\n";

    css += "QSpinBox::up-button, QSpinBox::down-button {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += "  width: 20px;\n";
    css += "}\n\n";

    css += "QSpinBox::up-button:hover, QSpinBox::down-button:hover {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += "}\n\n";

    // QTabWidget
    css += "QTabWidget::pane {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  border: %1px solid %2;\n").arg(panelBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  top: -1px;\n";
    css += "}\n\n";

    css += "QTabBar::tab {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 8px 16px;\n";
    css += "  margin-right: 2px;\n";
    css += "}\n\n";

    css += "QTabBar::tab:selected {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  color: %1;\n").arg(primary);
    css += "  border-bottom-color: transparent;\n";
    css += "}\n\n";

    css += "QTabBar::tab:hover {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += "}\n\n";

    // QGroupBox
    css += "QGroupBox {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(panelBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  margin-top: 12px;\n";
    css += "  padding: 12px;\n";
    css += "  font-weight: bold;\n";
    css += "}\n\n";

    css += "QGroupBox::title {\n";
    css += "  subcontrol-origin: margin;\n";
    css += "  left: 8px;\n";
    css += "  padding: 0 4px;\n";
    css += "}\n\n";

    // QScrollBar
    css += "QScrollBar:vertical {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += "  width: 14px;\n";
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QScrollBar::handle:vertical {\n";
    css += QString("  background-color: %1;\n").arg(border);
    css += "  min-height: 30px;\n";
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QScrollBar::handle:vertical:hover {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += "}\n\n";

    css += "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n";
    css += "  height: 0px;\n";
    css += "}\n\n";

    css += "QScrollBar:horizontal {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += "  height: 14px;\n";
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QScrollBar::handle:horizontal {\n";
    css += QString("  background-color: %1;\n").arg(border);
    css += "  min-width: 30px;\n";
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QScrollBar::handle:horizontal:hover {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += "}\n\n";

    // QTreeView/QTreeWidget
    css += "QTreeView, QTreeWidget {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "}\n\n";

    css += "QTreeView::item:selected, QTreeWidget::item:selected {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += QString("  color: %1;\n").arg(bg);
    css += "}\n\n";

    css += "QTreeView::item:hover, QTreeWidget::item:hover {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += "}\n\n";

    // QPlainTextEdit/QTextEdit (EXCLUDING ImageCanvas)
    css += "QPlainTextEdit, QTextEdit {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 6px;\n";
    css += "  font-family: monospace;\n";
    css += "}\n\n";

    // QMenuBar
    css += "QMenuBar {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  color: %1;\n").arg(text);
    css += "  border: none;\n";
    css += "  padding: 4px;\n";
    css += "}\n\n";

    css += "QMenuBar::item {\n";
    css += "  background-color: transparent;\n";
    css += "  padding: 6px 12px;\n";
    css += "}\n\n";

    css += "QMenuBar::item:selected {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += QString("  color: %1;\n").arg(bg);
    css += "}\n\n";

    // QMenu
    css += "QMenu {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 4px;\n";
    css += "}\n\n";

    css += "QMenu::item {\n";
    css += "  padding: 6px 24px;\n";
    css += "}\n\n";

    css += "QMenu::item:selected {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += QString("  color: %1;\n").arg(bg);
    css += "}\n\n";

    // QStatusBar
    css += "QStatusBar {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  color: %1;\n").arg(text);
    css += "  border: none;\n";
    css += "}\n\n";

    // QToolBar
    css += "QToolBar {\n";
    css += QString("  background-color: %1;\n").arg(panel);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += "  spacing: 4px;\n";
    css += "  padding: 4px;\n";
    css += "}\n\n";

    css += "QToolBar::handle {\n";
    css += QString("  background-color: %1;\n").arg(border);
    css += "  width: 8px;\n";
    css += "  margin: 4px;\n";
    css += "}\n\n";

    // QProgressBar
    css += "QProgressBar {\n";
    css += QString("  background-color: %1;\n").arg(panelAlt);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  text-align: center;\n";
    css += "}\n\n";

    css += "QProgressBar::chunk {\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += "}\n\n";

    // QToolTip
    css += "QToolTip {\n";
    css += QString("  background-color: %1;\n").arg(border);
    css += QString("  color: %1;\n").arg(text);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += QString("  border-radius: %1px;\n").arg(cornerRadius);
    css += "  padding: 6px;\n";
    css += "}\n\n";

    // QCheckBox - Retro pixel checkmark effect
    css += "/* Retro pixel checkmark effect */\n";
    css += "QCheckBox::indicator:checked {\n";
    css += "  image: none;\n";
    css += QString("  background-color: %1;\n").arg(primary);
    css += QString("  border: %1px solid %2;\n").arg(btnBorder).arg(border);
    css += "}\n\n";

    // Status indicators with color coding
    css += "/* Color-coded status classes */\n";
    css += ".status-error { color: " + error + "; }\n";
    css += ".status-warning { color: " + warning + "; }\n";
    css += ".status-success { color: " + success + "; }\n";
    css += ".status-info { color: " + color("accent_info").name() + "; }\n";

    return css;
}

void ThemeManager::applyTheme() {
    // Apply stylesheet to application
    qApp->setStyleSheet(generateStyleSheet());

    // Trigger update on all top-level widgets for hot reload
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (widget) {
            widget->update();

            // Also update all children
            for (QWidget* child : widget->findChildren<QWidget*>()) {
                if (child && !child->objectName().isEmpty()) {
                    child->style()->unpolish(child);
                    child->style()->polish(child);
                    child->update();
                }
            }
        }
    }

    qDebug() << "Theme applied:" << m_currentThemeId;
}

QString ThemeManager::themeAssetsPath(const QString& themeId) const {
    return QDir(m_assetsPath).filePath(themeId);
}

QString ThemeManager::widgetStyleSheet(const QString& widgetType) const {
    return QString();
}

} // namespace labelmaster::ui
