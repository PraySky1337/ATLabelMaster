/**
 * @file pixel_button.cpp
 * @brief Implementation of pixel button widgets
 */

#include "pixel_button.hpp"
#include "theme_manager.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>
#include <QDebug>
#include <QToolButton>
#include <QEnterEvent>

namespace labelmaster::ui {

//=============================================================================
// PixelButton
//=============================================================================

PixelButton::PixelButton(QWidget* parent)
    : QPushButton(parent) {
    initStyle();
}

PixelButton::PixelButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {
    initStyle();
}

PixelButton::PixelButton(const QIcon& icon, const QString& text, QWidget* parent)
    : QPushButton(icon, text, parent) {
    initStyle();
}

void PixelButton::initStyle() {
    // Remove default styling
    setAttribute(Qt::WA_TranslucentBackground, false);

    // Set cursor
    setCursor(Qt::PointingHandCursor);

    // Set font to pixel style
    QFont font = ThemeManager::instance().uiFont();
    font.setBold(true);
    setFont(font);
}

void PixelButton::setPixelBorderWidth(int width) {
    border_width_ = width;
    update();
}

int PixelButton::pixelBorderWidth() const {
    return border_width_;
}

void PixelButton::setShowPressedEffect(bool show) {
    show_pressed_effect_ = show;
    update();
}

bool PixelButton::showPressedEffect() const {
    return show_pressed_effect_;
}

void PixelButton::enterEvent(QEvent* event) {
    is_hovered_ = true;
    QPushButton::enterEvent(static_cast<QEnterEvent*>(event));
    update();
}

void PixelButton::leaveEvent(QEvent* event) {
    is_hovered_ = false;
    QPushButton::leaveEvent(event);
    update();
}

QColor PixelButton::getButtonColor() const {
    return ThemeManager::instance().color("accent_primary", QColor(255, 107, 107));
}

QColor PixelButton::getButtonHoverColor() const {
    return ThemeManager::instance().color("button_hover", QColor(255, 135, 135));
}

QColor PixelButton::getButtonPressedColor() const {
    return ThemeManager::instance().color("button_pressed", QColor(232, 74, 74));
}

QColor PixelButton::getBorderColor() const {
    return ThemeManager::instance().color("border", QColor(26, 26, 45));
}

QColor PixelButton::getTextColor() const {
    return ThemeManager::instance().color("background", QColor(44, 33, 55));
}

void PixelButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // No antialiasing for pixel style

    QRect rect = this->rect();

    // Determine colors based on state
    QColor bgColor;
    QColor borderColor = getBorderColor();

    if (!isEnabled()) {
        bgColor = ThemeManager::instance().color("panel", QColor(93, 64, 87));
        borderColor = ThemeManager::instance().color("border", QColor(26, 26, 45));
    } else if (isDown()) {
        bgColor = getButtonPressedColor();
        // Offset pressed effect
        if (show_pressed_effect_) {
            painter.translate(1, 1);
        }
    } else if (is_hovered_) {
        bgColor = getButtonHoverColor();
    } else {
        bgColor = getButtonColor();
    }

    // Draw background (no rounded corners for pixel style)
    painter.fillRect(rect, bgColor);

    // Draw border
    painter.setPen(QPen(borderColor, border_width_));
    painter.drawRect(rect.adjusted(0, 0, -1, -1)); // -1 to keep border inside bounds

    // Draw text
    if (!text().isEmpty()) {
        painter.setPen(getTextColor());
        painter.setFont(font());

        Qt::AlignmentFlag alignment = Qt::AlignCenter;

        // Handle icon + text
        if (!icon().isNull()) {
            // Calculate sizes
            QSize iconSize = this->iconSize();
            QSize textSize = painter.fontMetrics().size(Qt::TextSingleLine, text());
            int totalWidth = iconSize.width() + textSize.width() + 8;
            int startX = (rect.width() - totalWidth) / 2;

            // Draw icon
            QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
            QIcon::State state = isCheckable() && isChecked() ? QIcon::On : QIcon::Off;
            QPixmap pix = icon().pixmap(iconSize, mode);
            painter.drawPixmap(startX, (rect.height() - iconSize.height()) / 2, pix);

            // Draw text
            int textX = startX + iconSize.width() + 8;
            painter.drawText(QRect(textX, 0, rect.width() - textX, rect.height()),
                             alignment, text());
        } else {
            // Text only
            painter.drawText(rect, alignment, text());
        }
    } else if (!icon().isNull()) {
        // Icon only
        QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        QIcon::State state = isCheckable() && isChecked() ? QIcon::On : QIcon::Off;
        QPixmap pix = icon().pixmap(rect.size(), mode);
        painter.drawPixmap(rect, pix);
    }
}

//=============================================================================
// PixelToolButton
//=============================================================================

PixelToolButton::PixelToolButton(QWidget* parent)
    : QToolButton(parent) {
    setAttribute(Qt::WA_TranslucentBackground, false);
    setCursor(Qt::PointingHandCursor);
    setAutoRaise(false);

    QFont font = ThemeManager::instance().uiFont();
    font.setBold(false);
    setFont(font);
}

void PixelToolButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QRect rect = this->rect();
    auto& theme = ThemeManager::instance();

    // Determine colors
    QColor bgColor;
    QColor borderColor = theme.color("border", QColor(26, 26, 45));

    if (!this->isEnabled()) {
        bgColor = theme.color("panel", QColor(93, 64, 87));
    } else if (this->isDown() || this->isChecked()) {
        bgColor = theme.color("button_pressed", QColor(232, 74, 74));
    } else if (this->underMouse()) {
        bgColor = theme.color("button_hover", QColor(255, 135, 135));
    } else {
        bgColor = theme.color("panel", QColor(93, 64, 87));
    }

    // Draw background
    painter.fillRect(rect, bgColor);

    // Draw border
    painter.setPen(QPen(borderColor, 1));
    painter.drawRect(rect.adjusted(0, 0, -1, -1));

    // Draw icon
    if (!this->icon().isNull()) {
        QIcon::Mode mode = this->isEnabled() ? QIcon::Normal : QIcon::Disabled;
        QIcon::State state = this->isChecked() ? QIcon::On : QIcon::Off;
        QRect iconRect = rect.adjusted(2, 2, -2, -2);
        QPixmap pix = this->icon().pixmap(iconRect.size(), mode);
        painter.drawPixmap(iconRect, pix);
    }
}

} // namespace labelmaster::ui
