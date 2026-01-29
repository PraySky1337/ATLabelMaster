/**
 * @file svg_constants.cpp
 * @brief Implementation of SVG constants
 */

#include "svg_constants.hpp"

namespace labelmaster::util {

const ArmorTemplate& SvgConstants::smallArmor() {
    static ArmorTemplate template_{
        .width = 557.0,
        .height = 516.0,
        .anchors = QPolygonF{
            QPointF(0., 143.26),    // TL
            QPointF(0., 372.74),    // BL
            QPointF(557., 372.74),  // BR
            QPointF(557., 143.26)   // TR
        }
    };
    return template_;
}

const ArmorTemplate& SvgConstants::bigArmor() {
    static ArmorTemplate template_{
        .width = 871.0,
        .height = 478.0,
        .anchors = QPolygonF{
            QPointF(0., 140.61),    // TL
            QPointF(0., 347.39),    // BL
            QPointF(871., 347.39),  // BR
            QPointF(871., 140.61)   // TR
        }
    };
    return template_;
}

QColor SvgConstants::colorForClass(const QString& cls) {
    if (cls.isEmpty()) {
        return QColor(0, 200, 255);
    }
    return ColorMapper::colorForLetter(cls.left(1));
}

QColor ColorMapper::colorForLetter(const QString& colorLetter) {
    if (colorLetter.isEmpty()) {
        return QColor(0, 200, 255);  // Default cyan
    }

    const QChar c = colorLetter.at(0).toUpper();
    switch (c.toLatin1()) {
        case 'R':
            return QColor(255, 70, 70);         // Red
        case 'B':
            return QColor(61, 165, 255);       // Blue
        case 'G':
            return QColor(170, 170, 180);      // Gray
        case 'P':
            return QColor(255, 192, 203);      // Pink
        default:
            return QColor(0, 200, 255);        // Default cyan
    }
}

} // namespace labelmaster::util
