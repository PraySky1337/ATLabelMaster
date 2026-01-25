/**
 * @file keyboard_shortcuts.cpp
 * @brief Implementation of keyboard shortcut manager
 */

#include "keyboard_shortcuts.hpp"
#include "controller/settings.hpp"
#include <QSettings>

namespace labelmaster::util {

KeyboardManager& KeyboardManager::instance() {
    static KeyboardManager instance;
    return instance;
}

KeyboardManager::KeyboardManager() {
    initDefaults();
    load();
}

void KeyboardManager::initDefaults() {
    shortcuts_[KeyboardAction::OpenFolder] = QKeySequence(Qt::Key_O);
    shortcuts_[KeyboardAction::Save] = QKeySequence(Qt::Key_S);
    shortcuts_[KeyboardAction::Previous] = QKeySequence(Qt::Key_Q);
    shortcuts_[KeyboardAction::Next] = QKeySequence(Qt::Key_E);
    shortcuts_[KeyboardAction::SmartAnnotate] = QKeySequence(Qt::Key_Space);
    shortcuts_[KeyboardAction::HistogramEq] = QKeySequence(Qt::Key_H);
    shortcuts_[KeyboardAction::Delete] = QKeySequence(Qt::Key_Delete);
    shortcuts_[KeyboardAction::Settings] = QKeySequence(Qt::Key_F1);
    shortcuts_[KeyboardAction::Statistics] = QKeySequence();
    shortcuts_[KeyboardAction::Undo] = QKeySequence(Qt::CTRL | Qt::Key_Z);
    shortcuts_[KeyboardAction::Redo] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
    shortcuts_[KeyboardAction::ZoomIn] = QKeySequence(Qt::Key_Plus);
    shortcuts_[KeyboardAction::ZoomOut] = QKeySequence(Qt::Key_Minus);
    shortcuts_[KeyboardAction::ZoomReset] = QKeySequence(Qt::Key_0);
    shortcuts_[KeyboardAction::FitToWindow] = QKeySequence(Qt::Key_F);
    shortcuts_[KeyboardAction::ToggleGrid] = QKeySequence(Qt::Key_G);
    shortcuts_[KeyboardAction::PixelSnap] = QKeySequence(Qt::Key_P);
}

QKeySequence KeyboardManager::shortcut(KeyboardAction action) const {
    return shortcuts_.value(action);
}

void KeyboardManager::setShortcut(KeyboardAction action, const QKeySequence& sequence) {
    shortcuts_[action] = sequence;
    emit shortcutChanged(action, sequence);
}

void KeyboardManager::resetToDefaults() {
    initDefaults();
    save();
}

void KeyboardManager::applyToAction(QAction* action, KeyboardAction keyAction) {
    if (action) {
        action->setShortcut(shortcut(keyAction));
    }
}

QString KeyboardManager::actionName(KeyboardAction action) const {
    switch (action) {
        case KeyboardAction::OpenFolder: return tr("Open Folder");
        case KeyboardAction::Save: return tr("Save Labels");
        case KeyboardAction::Previous: return tr("Previous Image");
        case KeyboardAction::Next: return tr("Next Image");
        case KeyboardAction::SmartAnnotate: return tr("Smart Annotate");
        case KeyboardAction::HistogramEq: return tr("Histogram Equalize");
        case KeyboardAction::Delete: return tr("Delete");
        case KeyboardAction::Settings: return tr("Settings");
        case KeyboardAction::Statistics: return tr("Statistics");
        case KeyboardAction::Undo: return tr("Undo");
        case KeyboardAction::Redo: return tr("Redo");
        case KeyboardAction::ZoomIn: return tr("Zoom In");
        case KeyboardAction::ZoomOut: return tr("Zoom Out");
        case KeyboardAction::ZoomReset: return tr("Reset Zoom");
        case KeyboardAction::FitToWindow: return tr("Fit to Window");
        case KeyboardAction::ToggleGrid: return tr("Toggle Grid");
        case KeyboardAction::PixelSnap: return tr("Toggle Pixel Snap");
    }
    return QString();
}

QList<KeyboardAction> KeyboardManager::allActions() const {
    return shortcuts_.keys();
}

void KeyboardManager::save() {
    QSettings settings;
    settings.beginGroup("keyboard_shortcuts");

    for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it) {
        QString key = QString("action_%1").arg(static_cast<int>(it.key()));
        settings.setValue(key, it.value().toString());
    }

    settings.endGroup();
}

void KeyboardManager::load() {
    QSettings settings;
    settings.beginGroup("keyboard_shortcuts");

    for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it) {
        QString key = QString("action_%1").arg(static_cast<int>(it.key()));
        if (settings.contains(key)) {
            QString seqStr = settings.value(key).toString();
            it.value() = QKeySequence(seqStr);
        }
    }

    settings.endGroup();
}

} // namespace labelmaster::util
