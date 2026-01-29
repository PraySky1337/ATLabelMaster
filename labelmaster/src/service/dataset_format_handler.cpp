/**
 * @file dataset_format_handler.cpp
 * @brief Implementation of dataset format handlers
 */

#include "dataset_format_handler.hpp"
#include "../util/id_convert.hpp"
#include "../util/string.hpp"
#include "../util/svg_constants.hpp"
#include <QtMath>
#include <QTransform>
#include <limits>

namespace labelmaster::service {

bool LabelMaster2Format::read(
    QTextStream& stream,
    QVector<Armor>& armors,
    const QSize& imgSize) {

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.simplified().split(' ');

        // Format: color size cls x0 y0 x1 y1 x2 y2 x3 y3
        if (parts.size() != 11)
            continue;

        Armor armor;
        armor.color = IdConvert::colorId2Letter(parts[0].toInt());
        armor.size = parts[1].toInt();
        armor.cls = IdConvert::idCollect2Token(parts[2].toInt());

        // Denormalize coordinates from [0,1] to pixel coords
        armor.p0 = QPointF(parts[3].toDouble() * W, parts[4].toDouble() * H);
        armor.p1 = QPointF(parts[5].toDouble() * W, parts[6].toDouble() * H);
        armor.p2 = QPointF(parts[7].toDouble() * W, parts[8].toDouble() * H);
        armor.p3 = QPointF(parts[9].toDouble() * W, parts[10].toDouble() * H);

        armors.append(armor);
    }

    return true;
}

bool LabelMaster2Format::write(
    QTextStream& stream,
    const QVector<Armor>& armors,
    const QSize& imgSize) {

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    for (const auto& armor : armors) {
        const int colorId = IdConvert::colorLetter2Id(armor.color);
        const int classId = IdConvert::classToken2Id(armor.cls);

        // Normalize to [0,1]
        QPointF q0(armor.p0.x() / W, armor.p0.y() / H);
        QPointF q1(armor.p1.x() / W, armor.p1.y() / H);
        QPointF q2(armor.p2.x() / W, armor.p2.y() / H);
        QPointF q3(armor.p3.x() / W, armor.p3.y() / H);

        stream << colorId << ' ' << armor.size << ' ' << classId << ' '
               << q0.x() << ' ' << q0.y() << ' '
               << q1.x() << ' ' << q1.y() << ' '
               << q2.x() << ' ' << q2.y() << ' '
               << q3.x() << ' ' << q3.y() << '\n';
    }

    return true;
}

bool HITSZFormat::read(
    QTextStream& stream,
    QVector<Armor>& armors,
    const QSize& imgSize) {

    // HITSZ format parsing - similar to LabelMaster2
    // For now, use the same parsing logic
    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.simplified().split(' ');

        if (parts.size() != 11)
            continue;

        Armor armor;
        armor.color = IdConvert::colorId2Letter(parts[0].toInt());
        armor.size = parts[1].toInt();
        armor.cls = IdConvert::idCollect2Token(parts[2].toInt());

        armor.p0 = QPointF(parts[3].toDouble() * W, parts[4].toDouble() * H);
        armor.p1 = QPointF(parts[5].toDouble() * W, parts[6].toDouble() * H);
        armor.p2 = QPointF(parts[7].toDouble() * W, parts[8].toDouble() * H);
        armor.p3 = QPointF(parts[9].toDouble() * W, parts[10].toDouble() * H);

        armors.append(armor);
    }

    return true;
}

bool HITSZFormat::write(
    QTextStream& stream,
    const QVector<Armor>& armors,
    const QSize& imgSize) {

    // HITSZ format writing - similar to LabelMaster2
    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    for (const auto& armor : armors) {
        const int colorId = IdConvert::colorLetter2Id(armor.color);
        const int classId = IdConvert::classToken2Id(armor.cls);

        QPointF q0(armor.p0.x() / W, armor.p0.y() / H);
        QPointF q1(armor.p1.x() / W, armor.p1.y() / H);
        QPointF q2(armor.p2.x() / W, armor.p2.y() / H);
        QPointF q3(armor.p3.x() / W, armor.p3.y() / H);

        stream << colorId << ' ' << armor.size << ' ' << classId << ' '
               << q0.x() << ' ' << q0.y() << ' '
               << q1.x() << ' ' << q1.y() << ' '
               << q2.x() << ' ' << q2.y() << ' '
               << q3.x() << ' ' << q3.y() << '\n';
    }

    return true;
}

bool ExtendedFormat::read(
    QTextStream& stream,
    QVector<Armor>& armors,
    const QSize& imgSize) {

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.simplified().split(' ');

        // Format: color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3
        if (parts.size() != 15)
            continue;

        Armor armor;
        armor.color = IdConvert::colorId2Letter(parts[0].toInt());
        armor.size = parts[1].toInt();
        armor.cls = IdConvert::idCollect2Token(parts[2].toInt());

        // bbox (stored for reference)
        armor.norm_x = parts[3].toDouble();
        armor.norm_y = parts[4].toDouble();
        armor.norm_w = parts[5].toDouble();
        armor.norm_h = parts[6].toDouble();

        // Denormalize keypoint coordinates
        armor.p0 = QPointF(parts[7].toDouble() * W, parts[8].toDouble() * H);
        armor.p1 = QPointF(parts[9].toDouble() * W, parts[10].toDouble() * H);
        armor.p2 = QPointF(parts[11].toDouble() * W, parts[12].toDouble() * H);
        armor.p3 = QPointF(parts[13].toDouble() * W, parts[14].toDouble() * H);

        armors.append(armor);
    }

    return true;
}

bool ExtendedFormat::write(
    QTextStream& stream,
    const QVector<Armor>& armors,
    const QSize& imgSize) {

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    for (const auto& armor : armors) {
        const int colorId = IdConvert::colorLetter2Id(armor.color);
        const int classId = IdConvert::classToken2Id(armor.cls);

        QPointF q0(armor.p0.x() / W, armor.p0.y() / H);
        QPointF q1(armor.p1.x() / W, armor.p1.y() / H);
        QPointF q2(armor.p2.x() / W, armor.p2.y() / H);
        QPointF q3(armor.p3.x() / W, armor.p3.y() / H);

        // Calculate bbox using SVG perspective transformation (same as file.cpp)
        double x, y, w, h;
        const auto& svgTemplate = (armor.size == 0)
            ? labelmaster::util::SvgConstants::smallArmor()
            : labelmaster::util::SvgConstants::bigArmor();

        // SVG rectangle corners (pixel coords)
        QPolygonF svg_quad;
        svg_quad << QPointF(0., 0.)
                 << QPointF(0., svgTemplate.height)
                 << QPointF(svgTemplate.width, svgTemplate.height)
                 << QPointF(svgTemplate.width, 0.);

        // Image anchor points (pixel coords)
        QPolygonF img_anchors;
        img_anchors << armor.p0 << armor.p1 << armor.p2 << armor.p3;

        QTransform transform;
        if (QTransform::quadToQuad(svgTemplate.anchors, img_anchors, transform)) {
            // Transform SVG rectangle to image space
            QPolygonF img_corners = transform.map(svg_quad);

            double min_x = std::numeric_limits<double>::max();
            double min_y = std::numeric_limits<double>::max();
            double max_x = std::numeric_limits<double>::lowest();
            double max_y = std::numeric_limits<double>::lowest();

            for (const auto& pt : img_corners) {
                min_x = std::min(min_x, pt.x());
                min_y = std::min(min_y, pt.y());
                max_x = std::max(max_x, pt.x());
                max_y = std::max(max_y, pt.y());
            }

            // Convert to normalized coordinates
            w = (max_x - min_x) / W;
            h = (max_y - min_y) / H;
            x = (min_x + max_x) / (2.0 * W);
            y = (min_y + max_y) / (2.0 * H);
        } else {
            // Fallback: simple bbox from corner points
            double min_x = std::min({armor.p0.x(), armor.p1.x(), armor.p2.x(), armor.p3.x()});
            double min_y = std::min({armor.p0.y(), armor.p1.y(), armor.p2.y(), armor.p3.y()});
            double max_x = std::max({armor.p0.x(), armor.p1.x(), armor.p2.x(), armor.p3.x()});
            double max_y = std::max({armor.p0.y(), armor.p1.y(), armor.p2.y(), armor.p3.y()});
            w = (max_x - min_x) / W;
            h = (max_y - min_y) / H;
            x = (min_x + max_x) / (2.0 * W);
            y = (min_y + max_y) / (2.0 * H);
        }

        // Clamp to [0,1]
        auto clamp01 = [](double v) { return std::clamp(v, 0.0, 1.0); };
        x = clamp01(x);
        y = clamp01(y);
        w = clamp01(w);
        h = clamp01(h);
        QPointF q0_clamped(clamp01(q0.x()), clamp01(q0.y()));
        QPointF q1_clamped(clamp01(q1.x()), clamp01(q1.y()));
        QPointF q2_clamped(clamp01(q2.x()), clamp01(q2.y()));
        QPointF q3_clamped(clamp01(q3.x()), clamp01(q3.y()));

        stream << colorId << ' ' << armor.size << ' ' << classId << ' '
               << x << ' ' << y << ' ' << w << ' ' << h << ' '
               << q0_clamped.x() << ' ' << q0_clamped.y() << ' '
               << q1_clamped.x() << ' ' << q1_clamped.y() << ' '
               << q2_clamped.x() << ' ' << q2_clamped.y() << ' '
               << q3_clamped.x() << ' ' << q3_clamped.y() << '\n';
    }

    return true;
}

bool LabelMasterV4Format::read(
    QTextStream& stream,
    QVector<Armor>& armors,
    const QSize& imgSize) {

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.simplified().split(' ');

        // Format: cls x_c y_c w h x0 y0 x1 y1 x2 y2 x3 y3
        // cls: color*size*num (e.g., "R1", "B2", "G3")
        if (parts.size() != 13)
            continue;

        Armor armor;
        QString cls = parts[0];

        // Parse color from first character
        if (!cls.isEmpty()) {
            armor.color = cls.at(0).toUpper(); // R, B, G, P
        }

        // Parse class token (could be like "R1" -> "R1", "B10" -> "B10")
        armor.cls = cls;

        // Store normalized bbox
        armor.norm_x = parts[1].toDouble();
        armor.norm_y = parts[2].toDouble();
        armor.norm_w = parts[3].toDouble();
        armor.norm_h = parts[4].toDouble();

        // Denormalize corner points to pixel coords
        armor.p0 = QPointF(parts[5].toDouble() * W, parts[6].toDouble() * H);
        armor.p1 = QPointF(parts[7].toDouble() * W, parts[8].toDouble() * H);
        armor.p2 = QPointF(parts[9].toDouble() * W, parts[10].toDouble() * H);
        armor.p3 = QPointF(parts[11].toDouble() * W, parts[12].toDouble() * H);

        // Store normalized corner points
        armor.norm_p0 = QPointF(parts[5].toDouble(), parts[6].toDouble());
        armor.norm_p1 = QPointF(parts[7].toDouble(), parts[8].toDouble());
        armor.norm_p2 = QPointF(parts[9].toDouble(), parts[10].toDouble());
        armor.norm_p3 = QPointF(parts[11].toDouble(), parts[12].toDouble());

        // Determine size based on bbox aspect ratio (heuristic)
        // Big armor typically has higher aspect ratio than small armor
        double aspectRatio = armor.norm_w / (armor.norm_h + 1e-6);
        armor.size = (aspectRatio > 2.5) ? 1 : 0;  // 1 = big, 0 = small

        armors.append(armor);
    }

    return true;
}

bool LabelMasterV4Format::write(
    QTextStream& stream,
    const QVector<Armor>& armors,
    const QSize& imgSize) {

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    for (const auto& armor : armors) {
        // Use cls directly (e.g., "R1", "B2")
        stream << armor.cls << ' ';

        // Calculate bbox using SVG perspective transformation (same as file.cpp)
        double x, y, w, h;
        const auto& svgTemplate = (armor.size == 0)
            ? labelmaster::util::SvgConstants::smallArmor()
            : labelmaster::util::SvgConstants::bigArmor();

        // Get normalized image anchors
        QPointF q0, q1, q2, q3;
        if (!armor.norm_p0.isNull()) {
            q0 = armor.norm_p0;
            q1 = armor.norm_p1;
            q2 = armor.norm_p2;
            q3 = armor.norm_p3;
        } else {
            q0 = QPointF(armor.p0.x() / W, armor.p0.y() / H);
            q1 = QPointF(armor.p1.x() / W, armor.p1.y() / H);
            q2 = QPointF(armor.p2.x() / W, armor.p2.y() / H);
            q3 = QPointF(armor.p3.x() / W, armor.p3.y() / H);
        }

        // SVG rectangle corners (normalized to [0,1])
        QPolygonF svg_quad;
        svg_quad << QPointF(0., 0.)
                 << QPointF(0., svgTemplate.height)
                 << QPointF(svgTemplate.width, svgTemplate.height)
                 << QPointF(svgTemplate.width, 0.);

        // Image anchor points (normalized)
        QPolygonF img_anchors;
        img_anchors << q0 << q1 << q2 << q3;

        QTransform transform;
        if (QTransform::quadToQuad(svgTemplate.anchors, img_anchors, transform)) {
            // Transform SVG rectangle to image space
            QPolygonF img_corners = transform.map(svg_quad);

            double min_x = std::numeric_limits<double>::max();
            double min_y = std::numeric_limits<double>::max();
            double max_x = std::numeric_limits<double>::lowest();
            double max_y = std::numeric_limits<double>::lowest();

            for (const auto& pt : img_corners) {
                min_x = std::min(min_x, pt.x());
                min_y = std::min(min_y, pt.y());
                max_x = std::max(max_x, pt.x());
                max_y = std::max(max_y, pt.y());
            }

            w = max_x - min_x;
            h = max_y - min_y;
            x = (min_x + max_x) / 2.0;
            y = (min_y + max_y) / 2.0;
        } else {
            // Fallback: simple bbox from corner points
            double x_min = qMin(qMin(q0.x(), q1.x()), qMin(q2.x(), q3.x()));
            double x_max = qMax(qMax(q0.x(), q1.x()), qMax(q2.x(), q3.x()));
            double y_min = qMin(qMin(q0.y(), q1.y()), qMin(q2.y(), q3.y()));
            double y_max = qMax(qMax(q0.y(), q1.y()), qMax(q2.y(), q3.y()));
            w = x_max - x_min;
            h = y_max - y_min;
            x = (x_min + x_max) / 2.0;
            y = (y_min + y_max) / 2.0;
        }

        // Clamp to [0,1]
        auto clamp01 = [](double v) { return std::clamp(v, 0.0, 1.0); };
        x = clamp01(x);
        y = clamp01(y);
        w = clamp01(w);
        h = clamp01(h);
        QPointF q0_clamped(clamp01(q0.x()), clamp01(q0.y()));
        QPointF q1_clamped(clamp01(q1.x()), clamp01(q1.y()));
        QPointF q2_clamped(clamp01(q2.x()), clamp01(q2.y()));
        QPointF q3_clamped(clamp01(q3.x()), clamp01(q3.y()));

        // Write bbox (center format) and corner points
        stream << x << ' ' << y << ' ' << w << ' ' << h << ' '
               << q0_clamped.x() << ' ' << q0_clamped.y() << ' '
               << q1_clamped.x() << ' ' << q1_clamped.y() << ' '
               << q2_clamped.x() << ' ' << q2_clamped.y() << ' '
               << q3_clamped.x() << ' ' << q3_clamped.y() << '\n';
    }

    return true;
}

} // namespace labelmaster::service
