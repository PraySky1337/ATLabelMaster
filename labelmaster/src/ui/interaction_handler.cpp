/**
 * @file interaction_handler.cpp
 * @brief Implementation of InteractionHandler
 */

#include "interaction_handler.hpp"
#include "image_renderer.hpp"
#include "annotation_manager.hpp"
#include "mask_manager.hpp"
#include <QWheelEvent>
#include <QtMath>

namespace labelmaster::ui {

InteractionHandler::InteractionHandler(
    ImageRenderer& renderer,
    AnnotationManager& annotations,
    MaskManager& masks,
    QWidget* parentWidget)
    : renderer_(renderer)
    , annotations_(annotations)
    , masks_(masks)
    , parentWidget_(parentWidget) {
}

bool InteractionHandler::mousePressEvent(QMouseEvent* e) {
    if (!e || !parentWidget_)
        return false;

    QPoint pos = e->pos();
    dragStartPos_ = pos;

    // Check for ROI or handle interactions first
    // (This would be expanded in full implementation)

    // Check for mask mode
    if (isMaskMode_) {
        // Handle mask creation
        return true;
    }

    // Check for handle drag on selected armor
    if (annotations_.selectedIndex() >= 0) {
        int handle = hitHandleOnSelected(pos);
        if (handle >= 0) {
            dragState_ = DragState::DraggingHandle;
            dragHandle_ = handle;
            return true;
        }
    }

    // Check for armor selection
    int hitIndex = hitDetection(pos);
    if (hitIndex >= 0) {
        annotations_.setSelectedIndex(hitIndex);
        return true;
    }

    // Start creating new rect
    dragState_ = DragState::CreatingRect;
    return true;
}

bool InteractionHandler::mouseMoveEvent(QMouseEvent* e) {
    if (!e)
        return false;

    mouseInside_ = true;

    if (dragState_ == DragState::DraggingHandle) {
        // Update handle position
        // (Full implementation would update armor point)
        return true;
    }

    if (dragState_ == DragState::CreatingRect) {
        // Update drag rect preview
        return true;
    }

    // Update hover state
    int handle = hitHandleOnSelected(e->pos());
    hoverHandle_ = handle;

    int hitIndex = hitDetection(e->pos());
    annotations_.setHoverIndex(hitIndex);

    return true;
}

bool InteractionHandler::mouseReleaseEvent(QMouseEvent* e) {
    if (!e)
        return false;

    if (dragState_ == DragState::CreatingRect) {
        // Finalize new armor annotation
        annotations_.createNewDetection();
    }

    dragState_ = DragState::None;
    dragHandle_ = -1;
    return true;
}

bool InteractionHandler::mouseDoubleClickEvent(QMouseEvent* e) {
    if (!e)
        return false;

    // Double click to edit selected armor info
    if (annotations_.selectedArmor()) {
        // Open info dialog
        return true;
    }
    return false;
}

bool InteractionHandler::wheelEvent(QWheelEvent* e) {
    if (!e)
        return false;

    // Handle zoom with wheel
    // (Full implementation would update renderer scale)
    return true;
}

void InteractionHandler::keyPressEvent(QKeyEvent* e) {
    if (!e)
        return;

    // Handle keyboard shortcuts
    // (Full implementation would handle delete, escape, etc.)
}

int InteractionHandler::hitHandleOnSelected(const QPoint& widgetPos) const {
    const Armor* armor = annotations_.selectedArmor();
    if (!armor)
        return -1;

    // Check each corner point (handle)
    // (Full implementation would check all 4 corners)
    return -1;
}

int InteractionHandler::hitDetection(const QPoint& widgetPos) const {
    const auto& detections = annotations_.detections();

    for (int i = 0; i < detections.size(); ++i) {
        QPolygonF poly;
        poly << renderer_.imageToWidget(detections[i].p0)
             << renderer_.imageToWidget(detections[i].p1)
             << renderer_.imageToWidget(detections[i].p2)
             << renderer_.imageToWidget(detections[i].p3);

        if (pointInsidePoly(poly, widgetPos)) {
            return i;
        }
    }
    return -1;
}

bool InteractionHandler::pointInsidePoly(const QPolygonF& poly, const QPointF& point) const {
    return poly.containsPoint(point, Qt::OddEvenFill);
}

} // namespace labelmaster::ui
