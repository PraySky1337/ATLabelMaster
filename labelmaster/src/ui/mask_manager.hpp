/**
 * @file mask_manager.hpp
 * @brief Manages mask regions for annotation
 *
 * Extracted from ImageCanvas to separate mask management concerns.
 */

#ifndef LABELMASTER_MASK_MANAGER_HPP
#define LABELMASTER_MASK_MANAGER_HPP

#include <QPolygonF>
#include <QRect>
#include <QVector>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace labelmaster::ui {

/**
 * @brief Manages mask regions
 *
 * Handles storage and rendering of mask areas.
 */
class MaskManager {
public:
    MaskManager() = default;

    // Mask access
    const QVector<QRect>& masks() const { return masks_; }
    void setMasks(const QVector<QRect>& masks) { masks_ = masks; }
    void clearMasks() { masks_.clear(); }
    void addMask(const QRect& mask) { masks_.append(mask); }
    bool hasMasks() const { return !masks_.isEmpty(); }

    // Hit testing
    int hitMask(const QPoint& widgetPos, const QWidget* widget) const;

    // Drawing
    void drawMasks(QPainter& painter, const QWidget* widget) const;

private:
    QVector<QRect> masks_;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_MASK_MANAGER_HPP
