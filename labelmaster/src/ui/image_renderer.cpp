/**
 * @file image_renderer.cpp
 * @brief Implementation of ImageRenderer
 */

#include "image_renderer.hpp"
#include <QLabel>
#include <QtMath>

namespace labelmaster::ui {

ImageRenderer::ImageRenderer(QWidget* parentWidget)
    : parentWidget_(parentWidget) {
}

void ImageRenderer::setImage(const QImage& img) {
    img_ = img;
    imgPath_.clear();
    resetView();
}

void ImageRenderer::resetView() {
    if (img_.isNull() || !parentWidget_) {
        scale_ = 1.0;
        pan_ = QPointF(0, 0);
        fitRect_ = QRectF();
        return;
    }

    // Calculate fit-to-window scale
    QSizeF widgetSize = parentWidget_->size();
    QSizeF imageSize = img_.size();

    double scaleX = widgetSize.width() / imageSize.width();
    double scaleY = widgetSize.height() / imageSize.height();
    scale_ = std::min(scaleX, scaleY);
    scale_ = std::clamp(scale_, kMinScale_, kMaxScale_);

    // Center the image
    double scaledWidth = imageSize.width() * scale_;
    double scaledHeight = imageSize.height() * scale_;
    pan_.setX((widgetSize.width() - scaledWidth) / 2.0);
    pan_.setY((widgetSize.height() - scaledHeight) / 2.0);

    updateFitRect();
}

QPointF ImageRenderer::widgetToImage(const QPointF& widgetPos) const {
    if (img_.isNull())
        return widgetPos;
    return (widgetPos - pan_) / scale_;
}

QPointF ImageRenderer::imageToWidget(const QPointF& imagePos) const {
    if (img_.isNull())
        return imagePos;
    return imagePos * scale_ + pan_;
}

QRectF ImageRenderer::imageRectOnWidget() const {
    if (img_.isNull())
        return QRectF();
    return QRectF(pan_, img_.size() * scale_);
}

QRect ImageRenderer::widgetRectToImageRect(const QRect& widgetRect) const {
    if (img_.isNull())
        return widgetRect;

    QPointF tl = widgetToImage(widgetRect.topLeft());
    QPointF br = widgetToImage(widgetRect.bottomRight());
    return QRect(tl.toPoint(), br.toPoint()).normalized();
}

QRect ImageRenderer::clampRectToImage(const QRect& rect) const {
    if (img_.isNull())
        return rect;

    QRect imgRect(QPoint(0, 0), img_.size());
    QRect clamped = rect.intersected(imgRect);
    return clamped;
}

void ImageRenderer::updateFitRect() {
    if (img_.isNull() || !parentWidget_) {
        fitRect_ = QRectF();
        return;
    }

    QSizeF widgetSize = parentWidget_->size();
    QSizeF imageSize = img_.size();

    double scaleX = widgetSize.width() / imageSize.width();
    double scaleY = widgetSize.height() / imageSize.height();
    double fitScale = std::min(scaleX, scaleY);
    fitScale = std::clamp(fitScale, kMinScale_, kMaxScale_);

    double scaledWidth = imageSize.width() * fitScale;
    double scaledHeight = imageSize.height() * fitScale;
    double x = (widgetSize.width() - scaledWidth) / 2.0;
    double y = (widgetSize.height() - scaledHeight) / 2.0;

    fitRect_ = QRectF(x, y, scaledWidth, scaledHeight);
}

} // namespace labelmaster::ui
