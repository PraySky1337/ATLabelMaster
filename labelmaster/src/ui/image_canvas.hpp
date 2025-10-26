#pragma once
#include <QLabel>
#include <QPixmap>
#include <QRect>

class ImageCanvas : public QLabel {
    Q_OBJECT
public:
    explicit ImageCanvas(QWidget* parent = nullptr); // <- 这里改成 QWidget*

    bool loadImage(const QString& path);
    void setImage(const QImage& img);
    QString currentImagePath() const { return imgPath_; }

    QRect roi() const { return roiImg_; }
    void clearRoi();

    void resetView();
    double scaleFactor() const { return scale_; }

signals:
    void roiChanged(const QRect& roiImg);

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    // ……其余成员保持你当前版本……
    // (img_, imgPath_, scale_, pan_, fitRect_, roiImg_ 等)
    // 省略：坐标换算/绘制/缩放/拖拽相关私有方法与成员
    QImage img_;
    QString imgPath_;
    double scale_ = 1.0;
    QPointF pan_{0, 0};
    QRectF fitRect_;
    QPoint lastMousePos_;
    bool panning_ = false;
    QRect roiImg_;
    bool draggingRoi_ = false;
    QPoint dragStartW_;
    QPoint dragEndW_;
    QPoint mousePosW_{-1, -1};
    bool mouseInside_       = false;
    const double kMinScale_ = 0.2;
    const double kMaxScale_ = 8.0;

    // 声明你已有的私有方法：
    void updateFitRect();
    QRectF imageRectOnWidget() const;
    QPointF widgetToImage(const QPointF& p) const;
    QPointF imageToWidget(const QPointF& p) const;
    QRect widgetRectToImageRect(const QRect& rw) const;
    QRect imageRectToWidgetRect(const QRect& ri) const;
    void drawCrosshair(QPainter& p) const;
    void drawRoi(QPainter& p) const;
};
