/**
 * @file interaction_handler.hpp
 * @brief Handles mouse and keyboard input interactions
 *
 * Extracted from ImageCanvas to separate interaction handling concerns.
 */

#ifndef LABELMASTER_INTERACTION_HANDLER_HPP
#define LABELMASTER_INTERACTION_HANDLER_HPP

#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>
#include "types.hpp"

class QWidget;

namespace labelmaster::ui {

class ImageRenderer;
class AnnotationManager;
class MaskManager;

/**
 * @brief Handles mouse and keyboard input interactions
 *
 * Manages user input state and delegates to appropriate handlers.
 */
class InteractionHandler {
public:
    enum class DragState {
        None,
        CreatingRect,
        DraggingRect,
        DraggingHandle,
        Panning
    };

    InteractionHandler(
        ImageRenderer& renderer,
        AnnotationManager& annotations,
        MaskManager& masks,
        QWidget* parentWidget);

    // Mouse events
    bool mousePressEvent(QMouseEvent* e);
    bool mouseMoveEvent(QMouseEvent* e);
    bool mouseReleaseEvent(QMouseEvent* e);
    bool mouseDoubleClickEvent(QMouseEvent* e);
    bool wheelEvent(QWheelEvent* e);
    void keyPressEvent(QKeyEvent* e);

    // State queries
    DragState dragState() const { return dragState_; }
    QPoint dragStartPos() const { return dragStartPos_; }
    int dragHandle() const { return dragHandle_; }

    // Mode flags
    bool isMaskMode() const { return isMaskMode_; }
    void setMaskMode(bool enable) { isMaskMode_ = enable; }

private:
    int hitHandleOnSelected(const QPoint& widgetPos) const;
    int hitDetection(const QPoint& widgetPos) const;
    bool pointInsidePoly(const QPolygonF& poly, const QPointF& point) const;

private:
    ImageRenderer& renderer_;
    AnnotationManager& annotations_;
    MaskManager& masks_;
    QWidget* parentWidget_;

    // Interaction state
    DragState dragState_ = DragState::None;
    QPoint dragStartPos_;
    QPoint lastMousePos_;
    bool mouseInside_ = false;
    int dragHandle_ = -1;
    int hoverHandle_ = -1;

    // Mode flags
    bool isMaskMode_ = false;

    // Constants
    static constexpr int kHandleRadius_ = 6;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_INTERACTION_HANDLER_HPP
