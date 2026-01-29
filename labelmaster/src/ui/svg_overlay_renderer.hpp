/**
 * @file svg_overlay_renderer.hpp
 * @brief SVG overlay rendering for armor templates
 *
 * Extracted from ImageCanvas to separate SVG rendering concerns.
 */

#ifndef LABELMASTER_SVG_OVERLAY_RENDERER_HPP
#define LABELMASTER_SVG_OVERLAY_RENDERER_HPP

#include <QHash>
#include <QPolygonF>
#include <QVector>
#include <QSvgRenderer>
#include "types.hpp"

class QWidget;
class QPainter;

namespace labelmaster::ui {

/**
 * @brief Renders SVG armor template overlays
 *
 * Handles SVG loading, caching, and perspective-transformed rendering.
 */
class SvgOverlayRenderer {
public:
    explicit SvgOverlayRenderer(QWidget* parentWidget);
    ~SvgOverlayRenderer();

    void setupSvg();
    void drawSvg(QPainter& painter, const QVector<Armor>& armors) const;

private:
    QWidget* parentWidget_;
    // SVG cache:[class_id][size_id] -> renderer
    // size_id: 0=small, 1=big
    QHash<int, QHash<int, QSvgRenderer*>> svgCache_;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_SVG_OVERLAY_RENDERER_HPP
