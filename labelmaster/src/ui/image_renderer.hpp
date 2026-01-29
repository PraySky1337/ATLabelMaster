/**
 * @file image_renderer.hpp
 * @brief Image rendering, scaling, and panning functionality
 *
 * Extracted from ImageCanvas to separate rendering concerns.
 */

#ifndef LABELMASTER_IMAGE_RENDERER_HPP
#define LABELMASTER_IMAGE_RENDERER_HPP

#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QWidget>

namespace labelmaster::ui {

/**
 * @brief Handles image display, scaling, and panning
 *
 * Manages coordinate transformations between widget space and image space.
 */
class ImageRenderer {
public:
    explicit ImageRenderer(QWidget* parentWidget);

    // Image management
    void setImage(const QImage& img);
    const QImage& currentImage() const { return img_; }
    QString currentImagePath() const { return imgPath_; }
    void setImagePath(const QString& path) { imgPath_ = path; }

    // View transformations
    void resetView();
    double scaleFactor() const { return scale_; }
    void setScale(double scale) { scale_ = scale; }
    QPointF pan() const { return pan_; }
    void setPan(const QPointF& pan) { pan_ = pan; }

    // Coordinate transformations
    QPointF widgetToImage(const QPointF& widgetPos) const;
    QPointF imageToWidget(const QPointF& imagePos) const;
    QRectF imageRectOnWidget() const;
    QRect widgetRectToImageRect(const QRect& widgetRect) const;
    QRect clampRectToImage(const QRect& rect) const;

    // Rendering helpers
    void updateFitRect();
    QRectF fitRect() const { return fitRect_; }

private:
    QWidget* parentWidget_;
    QImage img_;
    QString imgPath_;
    double scale_ = 1.0;
    QPointF pan_{0, 0};
    QRectF fitRect_;

    static constexpr double kMinScale_ = 0.2;
    static constexpr double kMaxScale_ = 8.0;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_IMAGE_RENDERER_HPP
