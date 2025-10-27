#include "image_canvas.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>

ImageCanvas::ImageCanvas(QWidget* parent) : QLabel(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(100, 80);
}

bool ImageCanvas::loadImage(const QString& path) {
    QImage tmp(path);
    if (tmp.isNull()) return false;
    setImage(tmp);
    imgPath_ = path;
    return true;
}

void ImageCanvas::setImage(const QImage& img) {
    img_ = img;
    imgPath_.clear();
    dets_.clear();

    // 模型输入等于整图时，自动用整图做 ROI
    if (!img_.isNull() && modelInputSize_.isValid()
        && modelInputSize_ == img_.size()) {
        roiImg_ = QRect(QPoint(0,0), img_.size());
        emit roiChanged(roiImg_);
        emit roiCommitted(roiImg_);
    } else {
        clearRoi();
    }
    resetView();
    update();
}

void ImageCanvas::setModelInputSize(const QSize& s) {
    modelInputSize_ = s.isValid() ? s : QSize();
    // 若已加载图像且刚好相等，直接设整图为 ROI
    if (!img_.isNull() && modelInputSize_.isValid() && modelInputSize_ == img_.size()) {
        roiImg_ = QRect(QPoint(0,0), img_.size());
        emit roiChanged(roiImg_);
        emit roiCommitted(roiImg_);
        update();
    }
}

void ImageCanvas::setRoiMode(RoiMode m) {
    roiMode_ = m;
    // 切到 Fixed 模式但没有模型输入大小 -> 回退到 Free
    if (roiMode_ == RoiMode::FixedToModelSize && !modelInputSize_.isValid())
        roiMode_ = RoiMode::Free;
    update();
}

void ImageCanvas::clearRoi() {
    roiImg_ = QRect();
    draggingRoi_ = false;
    emit roiChanged(roiImg_);
    update();
}

QImage ImageCanvas::cropRoi() const {
    if (img_.isNull() || roiImg_.isNull()) return {};
    QRect r = clampRectToImage(roiImg_);
    return img_.copy(r);
}

void ImageCanvas::resetView() {
    scale_ = 1.0;
    pan_ = {0, 0};
    updateFitRect();
}

void ImageCanvas::requestDetectFull() {
    if (!img_.isNull()) emit detectRequested(img_);
}

void ImageCanvas::requestDetectRoi() {
    const QImage crop = cropRoi();
    if (!crop.isNull()) emit detectRequested(crop);
}

void ImageCanvas::setDetections(const QVector<Armor>& dets) {
    dets_ = dets;
    update();
}

void ImageCanvas::clearDetections() {
    dets_.clear();
    update();
}

/* ===== 几何工具 ===== */

void ImageCanvas::updateFitRect() {
    if (img_.isNull()) { fitRect_ = QRectF(); return; }
    const QSizeF W = size();
    QSizeF is = img_.size();
    QSizeF sc = is;
    sc.scale(W, Qt::KeepAspectRatio);
    const QPointF off((W.width()-sc.width())/2.0, (W.height()-sc.height())/2.0);
    fitRect_ = QRectF(off, sc);
}

QRectF ImageCanvas::imageRectOnWidget() const {
    if (img_.isNull()) return {};
    const QPointF c = fitRect_.center();
    const QSizeF  s = fitRect_.size() * scale_;
    QRectF r(QPointF(0,0), s);
    r.moveCenter(c + pan_);
    return r;
}

QPointF ImageCanvas::widgetToImage(const QPointF& p) const {
    const QRectF R = imageRectOnWidget();
    if (img_.isNull() || R.isEmpty()) return {};
    const double sx = img_.width()  / R.width();
    const double sy = img_.height() / R.height();
    QPointF pi((p.x()-R.x())*sx, (p.y()-R.y())*sy);
    pi.setX(std::clamp(pi.x(), 0.0, double(img_.width()-1)));
    pi.setY(std::clamp(pi.y(), 0.0, double(img_.height()-1)));
    return pi;
}

QPointF ImageCanvas::imageToWidget(const QPointF& p) const {
    const QRectF R = imageRectOnWidget();
    if (img_.isNull() || R.isEmpty()) return {};
    const double sx = R.width()  / img_.width();
    const double sy = R.height() / img_.height();
    return QPointF(R.x() + p.x()*sx, R.y() + p.y()*sy);
}

QRect ImageCanvas::widgetRectToImageRect(const QRect& rw) const {
    const QPointF tl = widgetToImage(rw.topLeft());
    const QPointF br = widgetToImage(rw.bottomRight());
    QRect r = QRect(tl.toPoint(), br.toPoint()).normalized();
    return clampRectToImage(r);
}

QRect ImageCanvas::clampRectToImage(const QRect& r) const {
    if (img_.isNull()) return {};
    return r.intersected(QRect(0,0,img_.width(), img_.height()));
}

/* ===== 绘制 ===== */

void ImageCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (img_.isNull()) return;

    // 图像
    const QRectF R = imageRectOnWidget();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(R, img_);

    // 检测结果
    drawDetections(p);

    // ROI
    drawRoi(p);

    // 十字
    drawCrosshair(p);
}

void ImageCanvas::drawDetections(QPainter& p) const {
    if (dets_.isEmpty()) return;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRect(imageRectOnWidget());

    for (const auto& d : dets_) {
        // 多边形
        QPolygonF poly;
        poly << imageToWidget(d.p0) << imageToWidget(d.p1)
             << imageToWidget(d.p2) << imageToWidget(d.p3);
        QPen pen(QColor(0, 200, 255), 2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(poly);

        // 文本
        const QPointF tl = poly.boundingRect().topLeft();
        const QString text = d.score > 0.f
            ? QString("%1 (%.2f)").arg(d.cls).arg(d.score)
            : d.cls;
        QFont f = p.font(); f.setPointSizeF(f.pointSizeF()+1); p.setFont(f);
        p.setPen(Qt::yellow);
        p.drawText(tl + QPointF(2, -2), text);
    }
    p.restore();
}

void ImageCanvas::drawRoi(QPainter& p) const {
    if (roiImg_.isNull()) return;
    const QRect rw = QRect(imageToWidget(roiImg_.topLeft()).toPoint(),
                           imageToWidget(roiImg_.bottomRight()).toPoint()).normalized();

    // 遮罩
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,100));
    QPainterPath path; path.addRect(rect());
    QPainterPath hole; hole.addRect(rw);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPath(path - hole);
    p.restore();

    // 边框 + 尺寸
    p.setPen(QPen(Qt::yellow, 2));
    p.drawRect(rw);
    p.setPen(Qt::white);
    p.drawText(rw.adjusted(4,4,-4,-4), Qt::AlignLeft|Qt::AlignTop,
               QString("%1×%2").arg(roiImg_.width()).arg(roiImg_.height()));
}

void ImageCanvas::drawCrosshair(QPainter& p) const {
    if (!mouseInside_ || img_.isNull()) return;
    const QRectF R = imageRectOnWidget();
    if (!R.contains(mousePosW_)) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setClipRect(R);
    p.setPen(QPen(QColor(0,255,0,180), 1));
    p.drawLine(QPoint(mousePosW_.x(), int(R.top())), QPoint(mousePosW_.x(), int(R.bottom())));
    p.drawLine(QPoint(int(R.left()), mousePosW_.y()), QPoint(int(R.right()), mousePosW_.y()));
    p.restore();
}

/* ===== 交互 ===== */

void ImageCanvas::wheelEvent(QWheelEvent* e) {
    if (img_.isNull()) { e->accept(); return; }

    const QPointF cursorW = e->position();
    const QPointF beforeI = widgetToImage(cursorW);

    const double step = e->angleDelta().y() > 0 ? 1.15 : (1.0/1.15);
    const double newScale = std::clamp(scale_ * step, kMinScale_, kMaxScale_);

    // 缩放围绕光标
    const QRectF R_before = imageRectOnWidget();
    scale_ = newScale;
    const QPointF afterW  = imageToWidget(beforeI);
    pan_ += (cursorW - afterW);

    update();
    e->accept();
}

void ImageCanvas::mousePressEvent(QMouseEvent* e) {
    if (img_.isNull()) return;
    lastMousePos_ = e->pos();

    if (e->button() == Qt::LeftButton) {
        if (roiMode_ == RoiMode::FixedToModelSize && modelInputSize_.isValid()) {
            placeFixedRoiAt(e->pos());     // 固定尺寸放置/移动
            emit roiCommitted(roiImg_);
        } else {
            beginFreeRoi(e->pos());        // 自由框
        }
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
        const QPoint delta = e->pos() - lastMousePos_;
        pan_ += delta;
        lastMousePos_ = e->pos();
        update();
        return;
    }

    if (draggingRoi_) { // 仅 Free 模式
        updateFreeRoi(e->pos());
        return;
    }

    if (roiMode_ == RoiMode::FixedToModelSize && (e->buttons() & Qt::LeftButton)) {
        placeFixedRoiAt(e->pos());
        emit roiChanged(roiImg_);
        update();
        return;
    }

    update(); // 十字
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && draggingRoi_) {
        endFreeRoi();
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

/* ===== ROI 逻辑 ===== */

void ImageCanvas::beginFreeRoi(const QPoint& wpos) {
    draggingRoi_ = true;
    dragStartW_  = wpos;
    roiImg_      = QRect();
}

void ImageCanvas::updateFreeRoi(const QPoint& wpos) {
    QRect rw = QRect(dragStartW_, wpos).normalized();
    roiImg_ = widgetRectToImageRect(rw);
    emit roiChanged(roiImg_);
    update();
}

void ImageCanvas::endFreeRoi() {
    draggingRoi_ = false;
    if (!roiImg_.isNull()) emit roiCommitted(roiImg_);
    update();
}

void ImageCanvas::placeFixedRoiAt(const QPoint& wpos) {
    if (!modelInputSize_.isValid()) return;
    // 鼠标位置（widget） -> 原图坐标作为中心
    const QPointF cI = widgetToImage(wpos);
    QRect r(QPoint(int(cI.x() - modelInputSize_.width()/2.0),
                   int(cI.y() - modelInputSize_.height()/2.0)),
            modelInputSize_);
    roiImg_ = clampRectToImage(r);
    emit roiChanged(roiImg_);
}
