/**
 * @file keyboard_shortcuts.hpp
 * @brief Keyboard shortcut customization system
 */

#ifndef LABELMASTER_KEYBOARD_SHORTCUTS_HPP
#define LABELMASTER_KEYBOARD_SHORTCUTS_HPP

#include <QObject>
#include <QString>
#include <QKeySequence>
#include <QHash>
#include <QAction>

namespace labelmaster::util {

/**
 * @brief Default keyboard actions
 */
enum class KeyboardAction {
    OpenFolder,
    Save,
    Previous,
    Next,
    SmartAnnotate,
    HistogramEq,
    Delete,
    Settings,
    Statistics,
    Undo,
    Redo,
    ZoomIn,
    ZoomOut,
    ZoomReset,
    FitToWindow,
    ToggleGrid,
    PixelSnap
};

/**
 * @brief Keyboard shortcut manager
 *
 * Manages customizable keyboard shortcuts.
 */
class KeyboardManager : public QObject {
    Q_OBJECT

public:
    static KeyboardManager& instance();

    /**
     * @brief Get key sequence for an action
     */
    QKeySequence shortcut(KeyboardAction action) const;

    /**
     * @brief Set key sequence for an action
     */
    void setShortcut(KeyboardAction action, const QKeySequence& sequence);

    /**
     * @brief Reset to default shortcuts
     */
    void resetToDefaults();

    /**
     * @brief Apply shortcuts to actions
     */
    void applyToAction(QAction* action, KeyboardAction keyAction);

    /**
     * @brief Get display name for action
     */
    QString actionName(KeyboardAction action) const;

    /**
     * @brief Get all actions
     */
    QList<KeyboardAction> allActions() const;

    /**
     * @brief Save shortcuts to settings
     */
    void save();

    /**
     * @brief Load shortcuts from settings
     */
    void load();

signals:
    void shortcutChanged(KeyboardAction action, const QKeySequence& sequence);

private:
    KeyboardManager();
    ~KeyboardManager() override = default;

    void initDefaults();

private:
    QHash<KeyboardAction, QKeySequence> shortcuts_;
};

/**
 * @brief Helper to get shortcut string
 */
inline QString shortcutString(KeyboardAction action) {
    return KeyboardManager::instance().shortcut(action).toString();
}

} // namespace labelmaster::util

#endif // LABELMASTER_KEYBOARD_SHORTCUTS_HPP
