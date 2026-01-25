/**
 * @file pixel_slider.cpp
 * @brief Implementation of pixel slider widget
 */

#include "pixel_slider.hpp"
#include "theme_manager.hpp"
#include <QPainter>
#include <QStyleOptionSlider>
#include <QDebug>

namespace labelmaster::ui {

PixelSlider::PixelSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent) {
    initStyle();
}

PixelSlider::PixelSlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent) {
    initStyle();
}

void PixelSlider::initStyle() {
    // Remove default styling
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void PixelSlider::setHandleSize(int size) {
    handle_size_ = size;
    update();
}

int PixelSlider::handleSize() const {
    return handle_size_;
}

void PixelSlider::setGrooveHeight(int height) {
    groove_height_ = height;
    update();
}

int PixelSlider::grooveHeight() const {
    return groove_height_;
}

QRect PixelSlider::grooveRect() const {
    QRect rect = this->rect();

    if (orientation() == Qt::Horizontal) {
        int y = (rect.height() - groove_height_) / 2;
        return QRect(0, y, rect.width(), groove_height_);
    } else {
        int x = (rect.width() - groove_height_) / 2;
        return QRect(x, 0, groove_height_, rect.height());
    }
}

QRect PixelSlider::handleRect() const {
    QRect groove = grooveRect();
    int pos = 0;

    if (orientation() == Qt::Horizontal) {
        if (minimum() == maximum()) {
            pos = 0;
        } else {
            pos = (value() - minimum()) * (groove.width() - handle_size_) / (maximum() - minimum());
        }
        int x = groove.x() + pos;
        int y = (rect().height() - handle_size_) / 2;
        return QRect(x, y, handle_size_, handle_size_);
    } else {
        if (minimum() == maximum()) {
            pos = 0;
        } else {
            pos = (value() - minimum()) * (groove.height() - handle_size_) / (maximum() - minimum());
        }
        int x = (rect().width() - handle_size_) / 2;
        int y = groove.y() + groove.height() - handle_size_ - pos;
        return QRect(x, y, handle_size_, handle_size_);
    }
}

QColor PixelSlider::getGrooveColor() const {
    return ThemeManager::instance().color("border", QColor(26, 26, 45));
}

QColor PixelSlider::getHandleColor() const {
    return ThemeManager::instance().color("accent_primary", QColor(255, 107, 107));
}

QColor PixelSlider::getHandleHoverColor() const {
    return ThemeManager::instance().color("button_hover", QColor(255, 135, 135));
}

void PixelSlider::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Draw groove
    QRect groove = grooveRect();
    QColor grooveColor = getGrooveColor();
    painter.fillRect(groove, grooveColor);

    // Draw filled portion (from min to current value)
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect filledGroove = groove;
    if (orientation() == Qt::Horizontal) {
        int handleX = handleRect().center().x();
        filledGroove.setWidth(handleX - groove.x());
    } else {
        int handleBottom = handleRect().bottom();
        filledGroove.setTop(handleBottom);
        filledGroove.setHeight(groove.bottom() - handleBottom + 1);
    }

    QColor fillColor = ThemeManager::instance().color("accent_secondary", QColor(78, 205, 196));
    painter.fillRect(filledGroove, fillColor);

    // Draw handle (square, no rounded corners)
    QRect handle = handleRect();
    QColor handleColor = getHandleColor();

    if (isSliderDown()) {
        handleColor = ThemeManager::instance().color("button_pressed", QColor(232, 74, 74));
    }

    // Draw handle background
    painter.fillRect(handle, handleColor);

    // Draw handle border
    painter.setPen(QPen(getGrooveColor(), 2));
    painter.drawRect(handle.adjusted(0, 0, -1, -1));

    // Draw tick marks if enabled
    if (tickPosition() != NoTicks) {
        painter.setPen(QPen(ThemeManager::instance().color("text", QColor(247, 255, 247)), 1));

        int interval = tickInterval();
        if (interval <= 0) {
            interval = pageStep();
        }
        if (interval <= 0) {
            interval = 10;
        }

        for (int val = minimum(); val <= maximum(); val += interval) {
            int pos = 0;
            if (minimum() == maximum()) {
                pos = 0;
            } else {
                if (orientation() == Qt::Horizontal) {
                    pos = (val - minimum()) * groove.width() / (maximum() - minimum());
                } else {
                    pos = groove.height() - (val - minimum()) * groove.height() / (maximum() - minimum());
                }
            }

            if (orientation() == Qt::Horizontal) {
                // Draw tick below groove
                int x = groove.x() + pos;
                int y = groove.bottom() + 2;
                painter.drawLine(x, y, x, y + 4);
            } else {
                // Draw tick to the right of groove
                int x = groove.right() + 2;
                int y = groove.y() + pos;
                painter.drawLine(x, y, x + 4, y);
            }
        }
    }
}

} // namespace labelmaster::ui
