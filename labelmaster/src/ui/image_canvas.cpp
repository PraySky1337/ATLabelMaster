#include "image_canvas.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtMath>

ImageCanvas::ImageCanvas(QWidget* parent) : QLabel(parent) {  // <- 这里也用 QWidget*
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(100,80);
}
bool ImageCanvas::loadImage(const QString& path) {
    QImage tmp(path);
    if (tmp.isNull()) return false;
    img_ = std::move(tmp);
    imgPath_ = path;
    clearRoi();
    resetView();
    update();
    return true;
}

void ImageCanvas::setImage(const QImage& img) {
    img_ = img;
    imgPath_.clear();
    clearRoi();
    resetView();
    update();
}

void ImageCanvas::clearRoi() {
    roiImg_ = QRect();
    draggingRoi_ = false;
    update();
    emit roiChanged(roiImg_);
}

void ImageCanvas::resetView() {
    scale_ = 1.0;
    pan_ = {0,0};
    updateFitRect();
}

void ImageCanvas::updateFitRect() {
    if (img_.isNull()) { fitRect_ = QRectF(); return; }
    QSizeF W = size();
    QSizeF is = img_.size();
    QSizeF sc = is;
    sc.scale(W, Qt::KeepAspectRatio);
    QPointF off((W.width()-sc.width())/2.0, (W.height()-sc.height())/2.0);
    fitRect_ = QRectF(off, sc);
}

QRectF ImageCanvas::imageRectOnWidget() const {
    if (img_.isNull()) return QRectF();
    // 在 fitRect_ 基础上应用缩放和平移
    QPointF c = fitRect_.center();
    QSizeF  s = fitRect_.size()*scale_;
    QRectF r(QPointF(0,0), s);
    r.moveCenter(c + pan_);
    return r;
}

QPointF ImageCanvas::widgetToImage(const QPointF& p) const {
    QRectF R = imageRectOnWidget();
    if (img_.isNull() || R.isEmpty()) return {};
    double sx = img_.width()  / R.width();
    double sy = img_.height() / R.height();
    QPointF pi((p.x()-R.x())*sx, (p.y()-R.y())*sy);
    // clamp
    pi.setX(std::clamp(pi.x(), 0.0, double(img_.width()-1)));
    pi.setY(std::clamp(pi.y(), 0.0, double(img_.height()-1)));
    return pi;
}

QPointF ImageCanvas::imageToWidget(const QPointF& p) const {
    QRectF R = imageRectOnWidget();
    if (img_.isNull() || R.isEmpty()) return {};
    double sx = R.width()  / img_.width();
    double sy = R.height() / img_.height();
    return QPointF(R.x() + p.x()*sx, R.y() + p.y()*sy);
}

QRect ImageCanvas::widgetRectToImageRect(const QRect& rw) const {
    QPointF tl = widgetToImage(rw.topLeft());
    QPointF br = widgetToImage(rw.bottomRight());
    QRect r = QRect(tl.toPoint(), br.toPoint()).normalized();
    r = r.intersected(QRect(0,0,img_.width(), img_.height()));
    return r;
}

QRect ImageCanvas::imageRectToWidgetRect(const QRect& ri) const {
    QPointF tl = imageToWidget(ri.topLeft());
    QPointF br = imageToWidget(ri.bottomRight());
    return QRect(tl.toPoint(), br.toPoint()).normalized();
}

void ImageCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (img_.isNull()) return;

    // 画图像
    QRectF R = imageRectOnWidget();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(R, img_);

    // 画 ROI
    drawRoi(p);

    // 画十字
    drawCrosshair(p);
}

void ImageCanvas::drawRoi(QPainter& p) const {
    if (roiImg_.isNull()) return;
    QRect rw = imageRectToWidgetRect(roiImg_);
    // 遮罩
    QColor mask(0,0,0,100);
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(mask);
    QPainterPath path; path.addRect(rect());
    QPainterPath hole; hole.addRect(rw);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPath(path - hole);
    p.restore();

    // 边框
    p.setPen(QPen(Qt::yellow, 2));
    p.drawRect(rw);
    p.setPen(Qt::white);
    p.drawText(rw.adjusted(4,4,-4,-4), Qt::AlignLeft|Qt::AlignTop,
               QString("%1×%2").arg(roiImg_.width()).arg(roiImg_.height()));
}

void ImageCanvas::drawCrosshair(QPainter& p) const {
    if (!mouseInside_ || img_.isNull()) return;
    // 仅在图像区域内画
    QRectF R = imageRectOnWidget();
    if (!R.contains(mousePosW_)) return;

    p.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(QColor(0,255,0,180)); pen.setWidth(1);
    p.setPen(pen);

    // 垂直/水平线穿过鼠标位置，但裁剪到图像矩形
    p.save();
    p.setClipRect(R);
    p.drawLine(QPoint(mousePosW_.x(), int(R.top())), QPoint(mousePosW_.x(), int(R.bottom())));
    p.drawLine(QPoint(int(R.left()), mousePosW_.y()), QPoint(int(R.right()), mousePosW_.y()));
    p.restore();
}

void ImageCanvas::wheelEvent(QWheelEvent* e) {
    if (img_.isNull()) { e->accept(); return; }
    // 以光标为中心的缩放
    const QPointF cursorW = e->position();
    const QPointF beforeI = widgetToImage(cursorW);

    const double step = e->angleDelta().y() > 0 ? 1.15 : (1.0/1.15);
    double newScale = std::clamp(scale_ * step, kMinScale_, kMaxScale_);

    // 调整 pan_，保证光标下的图像点放大前后对齐
    QRectF R_before = imageRectOnWidget();
    scale_ = newScale;
    QRectF R_after  = imageRectOnWidget();

    // 让同一个 image 点在 widget 上的位置保持不变
    QPointF afterW = imageToWidget(beforeI);
    pan_ += (cursorW - afterW); // 微调平移，让缩放围绕光标

    update();
    e->accept();
}

void ImageCanvas::mousePressEvent(QMouseEvent* e) {
    if (img_.isNull()) return;
    lastMousePos_ = e->pos();

    if (e->button() == Qt::LeftButton) {
        // 开启 ROI 拖拽
        draggingRoi_ = true;
        dragStartW_ = dragEndW_ = e->pos();
    } else if (e->button() == Qt::MiddleButton) {
        panning_ = true;
        setCursor(Qt::ClosedHandCursor);
    } else if (e->button() == Qt::RightButton) {
        clearRoi();
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* e) {
    mousePosW_ = e->pos();
    mouseInside_ = rect().contains(mousePosW_);

    if (panning_) {
        QPoint delta = e->pos() - lastMousePos_;
        pan_ += delta;
        lastMousePos_ = e->pos();
        update();
        return;
    }
    if (draggingRoi_) {
        dragEndW_ = e->pos();
        // 临时 ROI：把当前拖拽框转成原图坐标
        QRect rW = QRect(dragStartW_, dragEndW_).normalized();
        roiImg_ = widgetRectToImageRect(rW);
        emit roiChanged(roiImg_);
        update();
        return;
    }
    update(); // 仅更新十字
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && draggingRoi_) {
        draggingRoi_ = false;
        update();
    }
    if (e->button() == Qt::MiddleButton && panning_) {
        panning_ = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ImageCanvas::leaveEvent(QEvent*) {
    mouseInside_ = false;
    update();
}

void ImageCanvas::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    updateFitRect();
    update();
}
