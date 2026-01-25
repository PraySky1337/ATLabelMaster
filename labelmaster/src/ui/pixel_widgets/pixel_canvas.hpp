/**
 * @file pixel_canvas.hpp
 * @brief Pixel art style canvas widget for image display and annotation
 */

#ifndef LABELMASTER_PIXEL_CANVAS_HPP
#define LABELMASTER_PIXEL_CANVAS_HPP

#include <QWidget>
#include <QImage>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>

namespace labelmaster::ui {

/**
 * @brief Pixel art style canvas for displaying and annotating images
 *
 * Features:
 * - Pixel-perfect zoom and pan
 * - Grid overlay (optional)
 * - Pixel art annotation rendering
 * - Theme-aware colors
 */
class PixelCanvas : public QWidget {
    Q_OBJECT

public:
    explicit PixelCanvas(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    QImage image() const;

    void setZoom(double zoom);
    double zoom() const;

    void setShowGrid(bool show);
    bool showGrid() const;

    void setGridSize(int size);
    int gridSize() const;

    void setPixelSnapEnabled(bool enabled);
    bool pixelSnapEnabled() const;

signals:
    void zoomChanged(double zoom);
    void imageClicked(const QPoint& pos);
    void imageDragged(const QPoint& delta);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initCanvas();

    QRect imageRect() const;
    QPoint imageToCanvas(const QPoint& imagePos) const;
    QPoint canvasToImage(const QPoint& canvasPos) const;

    void drawGrid(QPainter& painter, const QRect& viewport) const;
    void drawImage(QPainter& painter) const;

private:
    QImage image_;
    double zoom_ = 1.0;
    QPoint offset_;
    QPoint last_mouse_pos_;
    bool is_dragging_ = false;

    bool show_grid_ = false;
    int grid_size_ = 16;
    bool pixel_snap_ = true;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_PIXEL_CANVAS_HPP
