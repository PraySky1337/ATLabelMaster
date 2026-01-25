/**
 * @file pixel_dialog.cpp
 * @brief Implementation of pixel dialog base class
 */

#include "pixel_dialog.hpp"
#include "theme_manager.hpp"
#include <QShowEvent>
#include <QPainter>

namespace labelmaster::ui {

PixelDialog::PixelDialog(QWidget* parent)
    : QDialog(parent) {
    initDialog();
}

PixelDialog::PixelDialog(const QString& title, QWidget* parent)
    : QDialog(parent) {
    initDialog();
    setWindowTitle(title);
}

void PixelDialog::initDialog() {
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void PixelDialog::setDialogBorderWidth(int width) {
    border_width_ = width;
    update();
}

int PixelDialog::dialogBorderWidth() const {
    return border_width_;
}

void PixelDialog::setApplyThemeOnShow(bool apply) {
    apply_theme_on_show_ = apply;
}

bool PixelDialog::applyThemeOnShow() const {
    return apply_theme_on_show_;
}

void PixelDialog::showEvent(QShowEvent* event) {
    if (apply_theme_on_show_) {
        // Apply theme stylesheet on show
        ThemeManager::instance().applyTheme();
    }
    QDialog::showEvent(event);
}

void PixelDialog::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    auto& theme = ThemeManager::instance();

    // Draw dialog border (pixel style - sharp corners)
    QColor borderColor = theme.color("border", QColor(26, 26, 45));
    painter.setPen(QPen(borderColor, border_width_));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

} // namespace labelmaster::ui
