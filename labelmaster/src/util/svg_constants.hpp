/**
 * @file svg_constants.hpp
 * @brief SVG constants for armor template rendering
 *
 * Centralizes SVG dimensions, anchor points, and color mappings
 * used throughout the annotation system.
 */

#ifndef LABELMASTER_SVG_CONSTANTS_HPP
#define LABELMASTER_SVG_CONSTANTS_HPP

#include <QColor>
#include <QPointF>
#include <QPolygonF>

namespace labelmaster::util {

/**
 * @brief SVG template data for armor types
 */
struct ArmorTemplate {
    double width;
    double height;
    QPolygonF anchors;  // TL, BL, BR, TR in SVG coordinates
};

/**
 * @brief Centralized SVG constants
 *
 * Provides SVG dimensions and anchor points for different armor sizes.
 * These values are used for perspective transformation calculations.
 */
class SvgConstants {
public:
    /**
     * @brief Get SVG template for small armor
     * @return ArmorTemplate with small armor dimensions and anchors
     */
    static const ArmorTemplate& smallArmor();

    /**
     * @brief Get SVG template for big armor
     * @return ArmorTemplate with big armor dimensions and anchors
     */
    static const ArmorTemplate& bigArmor();

    /**
     * @brief Get color for a given class string
     * @param cls Class string (e.g., "R1", "B2", "G3", "P4")
     * @return QColor for rendering the annotation
     */
    static QColor colorForClass(const QString& cls);

private:
    SvgConstants() = default;
};

/**
 * @brief Color mappings for armor annotations
 *
 * Maps color prefix letters to their display colors.
 */
class ColorMapper {
public:
    /**
     * @brief Get display color for color letter
     * @param colorLetter Color letter (R/B/G/P or empty)
     * @return QColor for rendering
     */
    static QColor colorForLetter(const QString& colorLetter);

private:
    ColorMapper() = default;
};

} // namespace labelmaster::util

#endif // LABELMASTER_SVG_CONSTANTS_HPP
