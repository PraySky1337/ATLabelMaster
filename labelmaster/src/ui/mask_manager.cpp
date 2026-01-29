/**
 * @file mask_manager.cpp
 * @brief Implementation of MaskManager
 */

#include "mask_manager.hpp"
#include <QPainter>
#include <QPainterPath>

namespace labelmaster::ui {

int MaskManager::hitMask(const QPoint& widgetPos, const QWidget* widget) const {
    if (!widget)
        return -1;

    for (int i = 0; i < masks_.size(); ++i) {
        if (masks_[i].contains(widgetPos)) {
            return i;
        }
    }
    return -1;
}

void MaskManager::drawMasks(QPainter& painter, const QWidget* widget) const {
    if (masks_.isEmpty() || !widget)
        return;

    painter.setBrush(QColor(0, 0, 0, 100));
    QPainterPath path;
    path.addRect(widget->rect());

    QPainterPath hole;
    for (const QRect& mask : masks_) {
        hole.addRect(mask);
    }

    path = path.subtracted(hole);
    painter.drawPath(path);
}

} // namespace labelmaster::ui
