/**
 * @file pixel_checkbox.cpp
 * @brief Implementation of pixel checkbox and radio button widgets
 */

#include "pixel_checkbox.hpp"
#include "theme_manager.hpp"
#include <QPainter>
#include <QDebug>

namespace labelmaster::ui {

//=============================================================================
// PixelCheckBox
//=============================================================================

PixelCheckBox::PixelCheckBox(QWidget* parent)
    : QCheckBox(parent) {
    initStyle();
}

PixelCheckBox::PixelCheckBox(const QString& text, QWidget* parent)
    : QCheckBox(text, parent) {
    initStyle();
}

void PixelCheckBox::initStyle() {
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void PixelCheckBox::setBoxSize(int size) {
    box_size_ = size;
    update();
}

int PixelCheckBox::boxSize() const {
    return box_size_;
}

QRect PixelCheckBox::boxRect() const {
    QSize sz = sizeHint();
    QRect textRect = fontMetrics().boundingRect(text());

    int y = (sz.height() - box_size_) / 2;
    int spacing = 8;
    int x = 0;

    if (!text().isEmpty()) {
        x = 0;
    }

    return QRect(x, y, box_size_, box_size_);
}

void PixelCheckBox::drawCheckMark(QPainter& painter, const QRect& rect) const {
    // Pixel art checkmark - blocky style
    QColor checkColor = ThemeManager::instance().color("background", QColor(44, 33, 55));

    int w = rect.width();
    int h = rect.height();

    // Draw blocky checkmark pattern
    painter.fillRect(rect.left() + w * 0.2, rect.top() + h * 0.5, w * 0.2, h * 0.1, checkColor);
    painter.fillRect(rect.left() + w * 0.3, rect.top() + h * 0.4, w * 0.1, h * 0.2, checkColor);
    painter.fillRect(rect.left() + w * 0.4, rect.top() + h * 0.3, w * 0.1, h * 0.3, checkColor);
    painter.fillRect(rect.left() + w * 0.5, rect.top() + h * 0.2, w * 0.1, h * 0.4, checkColor);
    painter.fillRect(rect.left() + w * 0.6, rect.top() + h * 0.2, w * 0.2, h * 0.1, checkColor);
}

void PixelCheckBox::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    auto& theme = ThemeManager::instance();

    // Draw checkbox box
    QRect box = boxRect();
    QColor bgColor = theme.color("background", QColor(44, 33, 55));
    QColor borderColor = theme.color("border", QColor(26, 26, 45));

    if (isChecked()) {
        bgColor = theme.color("accent_primary", QColor(255, 107, 107));
        borderColor = theme.color("border", QColor(26, 26, 45));
    } else if (!isEnabled()) {
        bgColor = theme.color("panel", QColor(93, 64, 87));
    }

    // Fill box
    painter.fillRect(box, bgColor);

    // Draw border
    painter.setPen(QPen(borderColor, 2));
    painter.drawRect(box.adjusted(0, 0, -1, -1));

    // Draw checkmark if checked
    if (isChecked()) {
        drawCheckMark(painter, box);
    }

    // Draw text
    if (!text().isEmpty()) {
        painter.setPen(theme.color("text", QColor(247, 255, 247)));
        painter.setFont(font());

        QRect textRect(box.right() + 8, 0, width() - box.right() - 8, height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    }
}

//=============================================================================
// PixelRadioButton
//=============================================================================

PixelRadioButton::PixelRadioButton(QWidget* parent)
    : QRadioButton(parent) {
    initStyle();
}

PixelRadioButton::PixelRadioButton(const QString& text, QWidget* parent)
    : QRadioButton(text, parent) {
    initStyle();
}

void PixelRadioButton::initStyle() {
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void PixelRadioButton::setBoxSize(int size) {
    box_size_ = size;
    update();
}

int PixelRadioButton::boxSize() const {
    return box_size_;
}

QRect PixelRadioButton::boxRect() const {
    int y = (height() - box_size_) / 2;
    return QRect(0, y, box_size_, box_size_);
}

void PixelRadioButton::drawCheckedIndicator(QPainter& painter, const QRect& rect) const {
    // Pixel art radio indicator - solid inner square
    QColor indicatorColor = ThemeManager::instance().color("background", QColor(44, 33, 55));

    int margin = rect.width() * 0.25;
    QRect inner = rect.adjusted(margin, margin, -margin, -margin);
    painter.fillRect(inner, indicatorColor);
}

void PixelRadioButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    auto& theme = ThemeManager::instance();

    // Draw radio box (square for pixel style!)
    QRect box = boxRect();
    QColor bgColor = theme.color("background", QColor(44, 33, 55));
    QColor borderColor = theme.color("border", QColor(26, 26, 45));

    if (!isEnabled()) {
        bgColor = theme.color("panel", QColor(93, 64, 87));
    }

    // Fill box
    painter.fillRect(box, bgColor);

    // Draw border
    painter.setPen(QPen(borderColor, 2));
    painter.drawRect(box.adjusted(0, 0, -1, -1));

    // Draw indicator if checked
    if (isChecked()) {
        drawCheckedIndicator(painter, box);
    }

    // Draw text
    if (!text().isEmpty()) {
        painter.setPen(theme.color("text", QColor(247, 255, 247)));
        painter.setFont(font());

        QRect textRect(box.right() + 8, 0, width() - box.right() - 8, height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    }
}

} // namespace labelmaster::ui
