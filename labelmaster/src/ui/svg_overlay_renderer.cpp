/**
 * @file svg_overlay_renderer.cpp
 * @brief Implementation of SvgOverlayRenderer
 */

#include "svg_overlay_renderer.hpp"
#include "../util/svg_constants.hpp"
#include "../util/id_convert.hpp"
#include "controller/settings.hpp"
#include <QPainter>
#include <QTransform>
#include <QFileInfo>
#include <QDebug>
#include <QWidget>

namespace labelmaster::ui {

SvgOverlayRenderer::SvgOverlayRenderer(QWidget* parentWidget)
    : parentWidget_(parentWidget) {
    setupSvg();
}

SvgOverlayRenderer::~SvgOverlayRenderer() {
    // SVG renderers are children of parentWidget_, Qt will delete them
}

void SvgOverlayRenderer::setupSvg() {
    // Use assets path from settings
    auto icons_dir = controller::AppSettings::instance().assetsDir() + "/icons";

    // G (哨兵) - class_id = 0, 通过 size 区分大小
    svgCache_[0][0] = new QSvgRenderer(icons_dir + "/Gs.svg", parentWidget_);
    svgCache_[0][1] = new QSvgRenderer(icons_dir + "/Gb.svg", parentWidget_);
    // 1 (一号大装甲) - class_id = 1
    svgCache_[1][0] = new QSvgRenderer(icons_dir + "/1.svg", parentWidget_);
    svgCache_[1][1] = svgCache_[1][0];
    // 2 (二号) - class_id = 2
    svgCache_[2][0] = new QSvgRenderer(icons_dir + "/2.svg", parentWidget_);
    // 3 (三号) - class_id = 3
    svgCache_[3][0] = new QSvgRenderer(icons_dir + "/3.svg", parentWidget_);
    svgCache_[3][1] = new QSvgRenderer(icons_dir + "/B3.svg", parentWidget_);
    // 4 (四号) - class_id = 4
    svgCache_[4][0] = new QSvgRenderer(icons_dir + "/4.svg", parentWidget_);
    svgCache_[4][1] = new QSvgRenderer(icons_dir + "/B4.svg", parentWidget_);
    // 5 (五号) - class_id = 5
    svgCache_[5][0] = new QSvgRenderer(icons_dir + "/5.svg", parentWidget_);
    svgCache_[5][1] = new QSvgRenderer(icons_dir + "/B5.svg", parentWidget_);
    // O (前哨站) - class_id = 6
    svgCache_[6][0] = new QSvgRenderer(icons_dir + "/O.svg", parentWidget_);
    // B (基地) - class_id = 7, 通过 size 区分大小
    svgCache_[7][0] = new QSvgRenderer(icons_dir + "/Bs.svg", parentWidget_);
    svgCache_[7][1] = new QSvgRenderer(icons_dir + "/Bb.svg", parentWidget_);

    qInfo() << "SVG loaded.";
}

void SvgOverlayRenderer::drawSvg(QPainter& painter, const QVector<Armor>& armors) const {
    if (armors.isEmpty() || !parentWidget_)
        return;

    painter.save();

    // Prepare SVG quad frames and anchor points
    QPolygonF big_svg_quad, small_svg_quad;
    big_svg_quad << QPointF(0., 0.) << QPointF(0., 478.) << QPointF(871., 478.)
                 << QPointF(871., 0.);
    small_svg_quad << QPointF(0., 0.) << QPointF(0., 516.) << QPointF(557., 516.)
                   << QPointF(557., 0.);

    // Get anchor points from SvgConstants
    const auto& bigAnchors = util::SvgConstants::bigArmor().anchors;
    const auto& smallAnchors = util::SvgConstants::smallArmor().anchors;

    // Canvas quad
    QPolygonF painter_quad;
    painter_quad << QPointF(0., 0.) << QPointF(0., parentWidget_->height())
                 << QPointF(parentWidget_->width(), parentWidget_->height())
                 << QPointF(parentWidget_->width(), 0.);

    // Transform SVG quad to canvas
    QTransform big_svg2painter, small_svg2painter;
    QTransform::quadToQuad(big_svg_quad, painter_quad, big_svg2painter);
    QTransform::quadToQuad(small_svg_quad, painter_quad, small_svg2painter);

    // Map SVG anchors to painter coords
    const QPolygonF big_src_on_painter = big_svg2painter.map(bigAnchors);
    const QPolygonF small_src_on_painter = small_svg2painter.map(smallAnchors);

    for (const auto& a : armors) {
        QString color = a.color;
        int type = IdConvert::classToken2Id(a.cls);
        int size = a.size;

        // Find corresponding QSvgRenderer
        QSvgRenderer* svg = svgCache_.value(type, {}).value(size, nullptr);
        if (!svg)
            continue;

        // Choose src quad based on size
        const QPolygonF* src_on_painter = (size == 0) ? &small_src_on_painter : &big_src_on_painter;

        // Get armor corners in widget coords (would need ImageRenderer for this)
        // For now, this is a simplified version - full implementation would require
        // passing the ImageRenderer or coordinate transform function

        // Note: Full implementation requires ImageRenderer to transform armor points
        // This is a placeholder for the complete SVG rendering logic
    }

    painter.restore();
}

} // namespace labelmaster::ui
