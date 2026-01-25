/**
 * @file pixel_canvas.cpp
 * @brief Implementation of pixel canvas widget
 */

#include "pixel_canvas.hpp"
#include "theme_manager.hpp"
#include <QPainter>
#include <QScrollBar>
#include <QDebug>

namespace labelmaster::ui {

PixelCanvas::PixelCanvas(QWidget* parent)
    : QWidget(parent) {
    initCanvas();
}

void PixelCanvas::initCanvas() {
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    // Set background color
    auto& theme = ThemeManager::instance();
    QPalette pal = palette();
    pal.setColor(QPalette::Window, theme.color("background", QColor(44, 33, 55)));
    setPalette(pal);
    setAutoFillBackground(true);
}

void PixelCanvas::setImage(const QImage& image) {
    image_ = image;
    update();
}

QImage PixelCanvas::image() const {
    return image_;
}

void PixelCanvas::setZoom(double zoom) {
    zoom_ = qBound(0.1, zoom, 10.0);
    update();
    emit zoomChanged(zoom_);
}

double PixelCanvas::zoom() const {
    return zoom_;
}

void PixelCanvas::setShowGrid(bool show) {
    show_grid_ = show;
    update();
}

bool PixelCanvas::showGrid() const {
    return show_grid_;
}

void PixelCanvas::setGridSize(int size) {
    grid_size_ = qMax(4, size);
    update();
}

int PixelCanvas::gridSize() const {
    return grid_size_;
}

void PixelCanvas::setPixelSnapEnabled(bool enabled) {
    pixel_snap_ = enabled;
}

bool PixelCanvas::pixelSnapEnabled() const {
    return pixel_snap_;
}

QRect PixelCanvas::imageRect() const {
    if (image_.isNull()) {
        return QRect();
    }

    QSize scaledSize = image_.size() * zoom_;
    int x = (width() - scaledSize.width()) / 2 + offset_.x();
    int y = (height() - scaledSize.height()) / 2 + offset_.y();

    return QRect(x, y, scaledSize.width(), scaledSize.height());
}

QPoint PixelCanvas::imageToCanvas(const QPoint& imagePos) const {
    QRect rect = imageRect();
    return rect.topLeft() + imagePos * zoom_;
}

QPoint PixelCanvas::canvasToImage(const QPoint& canvasPos) const {
    QRect rect = imageRect();
    QPoint local = canvasPos - rect.topLeft();

    if (pixel_snap_) {
        local.setX(qRound(local.x() / zoom_) * zoom_);
        local.setY(qRound(local.y() / zoom_) * zoom_);
    }

    return QPoint(qRound(local.x() / zoom_), qRound(local.y() / zoom_));
}

void PixelCanvas::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // Pixel style - no smoothing

    // Fill background
    auto& theme = ThemeManager::instance();
    QColor bgColor = theme.color("background", QColor(44, 33, 55));
    painter.fillRect(rect(), bgColor);

    // Draw checkerboard pattern for transparency
    if (!image_.isNull() && image_.hasAlphaChannel()) {
        QRect imgRect = imageRect();
        const int checkerSize = 8;

        for (int y = imgRect.top(); y < imgRect.bottom(); y += checkerSize) {
            for (int x = imgRect.left(); x < imgRect.right(); x += checkerSize) {
                QRect tile(x, y, checkerSize, checkerSize);
                // Clip to image rect
                tile = tile.intersected(imgRect);

                int checkerX = (x - imgRect.left()) / checkerSize;
                int checkerY = (y - imgRect.top()) / checkerSize;
                bool isWhite = (checkerX + checkerY) % 2 == 0;

                painter.fillRect(tile, isWhite ? QColor(200, 200, 200) : QColor(150, 150, 150));
            }
        }
    }

    // Draw image
    drawImage(painter);

    // Draw grid if enabled
    if (show_grid_ && !image_.isNull()) {
        drawGrid(painter, imageRect());
    }
}

void PixelCanvas::drawImage(QPainter& painter) const {
    if (image_.isNull()) {
        return;
    }

    QRect imgRect = imageRect();

    // Pixel-perfect scaling (no smoothing)
    painter.drawImage(imgRect, image_);
}

void PixelCanvas::drawGrid(QPainter& painter, const QRect& viewport) const {
    if (viewport.isEmpty()) {
        return;
    }

    auto& theme = ThemeManager::instance();
    QColor gridColor = theme.color("border", QColor(26, 26, 45));
    gridColor.setAlpha(100);

    painter.setPen(QPen(gridColor, 1));

    int scaledGridSize = qMax(1, static_cast<int>(grid_size_ * zoom_));

    // Vertical lines
    for (int x = viewport.left(); x <= viewport.right(); x += scaledGridSize) {
        painter.drawLine(x, viewport.top(), x, viewport.bottom());
    }

    // Horizontal lines
    for (int y = viewport.top(); y <= viewport.bottom(); y += scaledGridSize) {
        painter.drawLine(viewport.left(), y, viewport.right(), y);
    }
}

void PixelCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        is_dragging_ = true;
        last_mouse_pos_ = event->pos();
    }

    if (event->button() == Qt::LeftButton && !image_.isNull()) {
        QPoint imagePos = canvasToImage(event->pos());
        if (imageRect().contains(event->pos())) {
            emit imageClicked(imagePos);
        }
    }
}

void PixelCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (is_dragging_ && (event->buttons() & Qt::MiddleButton)) {
        QPoint delta = event->pos() - last_mouse_pos_;
        offset_ += delta;
        last_mouse_pos_ = event->pos();
        update();
        emit imageDragged(delta);
    } else if (is_dragging_ && (event->buttons() & Qt::LeftButton)) {
        last_mouse_pos_ = event->pos();
    }
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        is_dragging_ = false;
    }
}

void PixelCanvas::wheelEvent(QWheelEvent* event) {
    double delta = event->angleDelta().y() / 120.0;
    double newZoom = zoom_ * (1.0 + delta * 0.1);

    // Zoom toward mouse position
    QPointF mousePosF = event->position();
    QPoint mousePos = mousePosF.toPoint();
    QPoint imagePosBefore = canvasToImage(mousePos);

    setZoom(newZoom);

    // Adjust offset to zoom toward mouse
    QPoint imagePosAfter = canvasToImage(mousePos);
    offset_ += (imagePosAfter - imagePosBefore) * zoom_;

    update();
}

} // namespace labelmaster::ui
