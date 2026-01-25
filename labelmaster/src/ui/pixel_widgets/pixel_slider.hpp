/**
 * @file pixel_slider.hpp
 * @brief Pixel art style slider widget for ATLabelMaster
 *
 * Custom QSlider subclass with pixel art styling.
 */

#ifndef LABELMASTER_PIXEL_SLIDER_HPP
#define LABELMASTER_PIXEL_SLIDER_HPP

#include <QSlider>
#include <QPaintEvent>
#include <QEvent>

namespace labelmaster::ui {

/**
 * @brief Pixel art style slider
 *
 * A QSlider subclass that renders in pixel art style.
 * Features:
 * - Block-style handle (no rounded corners)
 * - Pixel-perfect groove
 * - Theme-aware colors
 * - Optional tick marks in pixel style
 */
class PixelSlider : public QSlider {
    Q_OBJECT

public:
    explicit PixelSlider(QWidget* parent = nullptr);
    explicit PixelSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

    void setHandleSize(int size);
    int handleSize() const;

    void setGrooveHeight(int height);
    int grooveHeight() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void initStyle();

    QRect handleRect() const;
    QRect grooveRect() const;

    QColor getGrooveColor() const;
    QColor getHandleColor() const;
    QColor getHandleHoverColor() const;

private:
    int handle_size_ = 18;
    int groove_height_ = 6;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_PIXEL_SLIDER_HPP
