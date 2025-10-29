#include "image_canvas.hpp"
#include "controller/settings.hpp"

#include <QFile>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QTransform>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <array>

// ---------- JSON 工具 ----------
static QJsonArray toJsonPt(const QPointF& p) { return QJsonArray{p.x(), p.y()}; }
static QPointF fromJsonPt(const QJsonArray& a) {
    return (a.size() == 2) ? QPointF(a.at(0).toDouble(), a.at(1).toDouble()) : QPointF{};
}
static QJsonObject armorToJson(const Armor& a) {
    QJsonObject o;
    o["cls"]   = a.cls;
    o["score"] = a.score;
    o["p0"]    = toJsonPt(a.p0);
    o["p1"]    = toJsonPt(a.p1);
    o["p2"]    = toJsonPt(a.p2);
    o["p3"]    = toJsonPt(a.p3);
    return o;
}

static bool armorFromJson(const QJsonObject& o, Armor& a) {
    if (!o.contains("cls") || !o.contains("p0") || !o.contains("p1") || !o.contains("p2")
        || !o.contains("p3"))
        return false;
    a.cls   = o.value("cls").toString();
    a.score = float(o.value("score").toDouble(0.0));
    a.p0    = fromJsonPt(o.value("p0").toArray());
    a.p1    = fromJsonPt(o.value("p1").toArray());
    a.p2    = fromJsonPt(o.value("p2").toArray());
    a.p3    = fromJsonPt(o.value("p3").toArray());
    return true;
}

// ---------- 构造 ----------
ImageCanvas::ImageCanvas(QWidget* parent)
    : QLabel(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(100, 80);
    setContextMenuPolicy(Qt::NoContextMenu); // 防止右键被菜单吃掉
    setupSvg();                              // 初始化SVG

    qRegisterMetaType<Armor>("ImageCanvas::Armor");
    qRegisterMetaType<QVector<Armor>>("QVector<ImageCanvas::Armor>");
}

/* ===== 图像 & 视图 ===== */

bool ImageCanvas::loadImage(const QString& path) {
    QImage tmp(path);
    if (tmp.isNull())
        return false;
    setImage(tmp);
    imgPath_ = path;
    return true;
}

void ImageCanvas::setImage(const QImage& img) {
    img_ = img;
    imgPath_.clear();

    // 切图即清空标注
    clearDetections();
    selectedIndex_ = -1;
    hoverIndex_    = -1;
    draggingRect_  = false;
    dragHandle_    = -1;
    hoverHandle_   = -1;
    dragRectImg_   = QRect();

    if (!img_.isNull() && modelInputSize_.isValid() && modelInputSize_ == img_.size()) {
        roiImg_ = QRect(QPoint(0, 0), img_.size());
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
    if (!img_.isNull() && modelInputSize_.isValid() && modelInputSize_ == img_.size()) {
        roiImg_ = QRect(QPoint(0, 0), img_.size());
        emit roiChanged(roiImg_);
        emit roiCommitted(roiImg_);
        update();
    }
}

void ImageCanvas::setRoiMode(RoiMode m) {
    roiMode_ = m;
    if (roiMode_ == RoiMode::FixedToModelSize && !modelInputSize_.isValid())
        roiMode_ = RoiMode::Free;
    update();
}

void ImageCanvas::clearRoi() {
    roiImg_      = QRect();
    draggingRoi_ = false;
    emit roiChanged(roiImg_);
    update();
}

QImage ImageCanvas::cropRoi() const {
    if (img_.isNull() || roiImg_.isNull())
        return {};
    return img_.copy(clampRectToImage(roiImg_));
}

void ImageCanvas::resetView() {
    scale_ = 1.0;
    pan_   = {0, 0};
    updateFitRect();
}

/* ===== 检测请求 ===== */
void ImageCanvas::requestDetect() {
    const QImage crop = cropRoi();
    if (!crop.isNull())
        emit detectRequested(crop);
    else
        emit detectRequested(img_);
}

/* ===== 外部读写 ===== */
void ImageCanvas::setDetections(const QVector<Armor>& dets) {
    qDebug() << "setDetections: " << dets.size();
    dets_ = dets;
    if (dets_.isEmpty()) {
        qDebug() << "setDetections: empty";
        selectedIndex_ = -1;
    } else if (selectedIndex_ >= dets_.size())
        selectedIndex_ = dets_.size() - 1;
    if (hoverIndex_ >= dets_.size()) {
        hoverIndex_ = -1;
        emit detectionHovered(-1);
    }
    emit detectionSelected(selectedIndex_);
    update();
}
void ImageCanvas::clearDetections() {
    dets_.clear();
    selectedIndex_ = -1;
    hoverIndex_    = -1;
    emit detectionSelected(-1);
    emit detectionHovered(-1);
    update();
}
void ImageCanvas::addDetection(const Armor& a0) {
    Armor a = a0;
    dets_.append(a);
    const int idx = dets_.size() - 1;
    emit detectionUpdated(idx, dets_.back());
    update();
}
void ImageCanvas::updateDetection(int index, const Armor& a0) {
    if (index < 0 || index >= dets_.size())
        return;
    dets_[index] = a0;
    emit detectionUpdated(index, dets_[index]);
    update();
}
void ImageCanvas::removeDetection(int index) {
    if (index < 0 || index >= dets_.size())
        return;
    dets_.removeAt(index);
    emit detectionRemoved(index);

    if (dets_.isEmpty()) {
        selectedIndex_ = -1;
        hoverIndex_    = -1;
    } else {
        if (selectedIndex_ == index)
            selectedIndex_ = -1;
        else if (selectedIndex_ > index)
            selectedIndex_ -= 1;

        if (hoverIndex_ == index)
            hoverIndex_ = -1;
        else if (hoverIndex_ > index)
            hoverIndex_ -= 1;
    }
    emit detectionSelected(selectedIndex_);
    emit detectionHovered(hoverIndex_);
    update();
}

bool ImageCanvas::setSelectedIndex(int idx) {
    if (idx < -1 || idx >= dets_.size())
        return false;
    selectedIndex_ = idx;
    dragHandle_ = hoverHandle_ = -1;
    emit detectionSelected(selectedIndex_);
    update();
    return true;
}
bool ImageCanvas::setSelectedClass(const QString& cls) {
    if (selectedIndex_ < 0 || selectedIndex_ >= dets_.size())
        return false;
    dets_[selectedIndex_].cls = cls.isEmpty() ? QStringLiteral("unknown") : cls;
    emit detectionUpdated(selectedIndex_, dets_[selectedIndex_]);
    update();
    return true;
}
void ImageCanvas::publishAnnotations() { emit annotationsPublished(imgPath_, roiImg_, dets_); }

/* ===== 导入/导出 ===== */
bool ImageCanvas::saveLabelsToJsonFile(const QString& path) const {
    QJsonObject root;
    if (!imgPath_.isEmpty())
        root["image_path"] = imgPath_;
    if (!roiImg_.isNull())
        root["roi"] = QJsonArray{roiImg_.x(), roiImg_.y(), roiImg_.width(), roiImg_.height()};
    QJsonArray arr;
    for (const auto& d : dets_)
        arr.push_back(armorToJson(d));
    root["objects"] = arr;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("saveLabelsToJsonFile: open('%s') failed", qPrintable(path));
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}
bool ImageCanvas::loadLabelsFromJsonFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("loadLabelsFromJsonFile: open('%s') failed", qPrintable(path));
        return false;
    }
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
        qWarning("loadLabelsFromJsonFile: json root is not object");
        return false;
    }
    const auto root = doc.object();

    clearDetections();
    selectedIndex_ = -1;
    hoverIndex_    = -1;

    if (root.contains("image_path"))
        imgPath_ = root.value("image_path").toString();

    if (root.contains("roi")) {
        const auto a = root.value("roi").toArray();
        if (a.size() == 4) {
            roiImg_ = QRect(a.at(0).toInt(), a.at(1).toInt(), a.at(2).toInt(), a.at(3).toInt());
            emit roiChanged(roiImg_);
            emit roiCommitted(roiImg_);
        }
    } else
        clearRoi();

    QVector<Armor> tmp;
    if (root.contains("objects")) {
        const auto arr = root.value("objects").toArray();
        tmp.reserve(arr.size());
        for (const auto& v : arr) {
            if (!v.isObject())
                continue;
            Armor a;
            if (armorFromJson(v.toObject(), a))
                tmp.push_back(a);
        }
    }
    setDetections(tmp);
    emit detectionSelected(-1);
    emit detectionHovered(-1);
    update();
    return true;
}

/* ===== 绘制 ===== */
void ImageCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    if (img_.isNull())
        return;

    const QRectF R = imageRectOnWidget();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(R, img_);

    drawDetections(p);
    drawRoi(p);
    drawSvg(p, dets_);
    drawDragRect(p); // <<< 新增：拖框时的虚线矩形
    drawCrosshair(p);
}

void ImageCanvas::drawDragRect(QPainter& p) const {
    if (!(draggingRect_ && !dragRectImg_.isNull()))
        return;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRect(imageRectOnWidget());

    const QRect rw = QRect(
                         imageToWidget(dragRectImg_.topLeft()).toPoint(),
                         imageToWidget(dragRectImg_.bottomRight()).toPoint())
                         .normalized();

    p.setPen(QPen(Qt::green, 2, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rw);
    p.restore();
}

void ImageCanvas::drawDetections(QPainter& p) const {
    if (dets_.isEmpty())
        return;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRect(imageRectOnWidget());

    for (int i = 0; i < dets_.size(); ++i) {
        const auto& d = dets_[i];
        QPolygonF poly;
        poly << imageToWidget(d.p0) << imageToWidget(d.p1) << imageToWidget(d.p2)
             << imageToWidget(d.p3);

        const bool isSel   = (i == selectedIndex_);
        const bool isHover = (i == hoverIndex_);

        if (isSel || isHover) {
            p.setPen(Qt::NoPen);
            p.setBrush(isSel ? QColor(255, 160, 0, 60) : QColor(0, 220, 255, 60));
            p.drawPolygon(poly);
        }

        QPen pen;
        if (isSel)
            pen = QPen(QColor(255, 120, 0), 3);
        else if (isHover)
            pen = QPen(QColor(0, 220, 255), 3);
        else
            pen = QPen(QColor(0, 200, 255), 2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(poly);

        const QPointF tl   = poly.boundingRect().topLeft();
        const QString text = d.score > 0.f ? QString("%1 (%.2f)").arg(d.cls).arg(d.score) : d.cls;
        QFont f            = p.font();
        f.setPointSizeF(f.pointSizeF() + 1);
        p.setFont(f);
        p.setPen(Qt::yellow);
        p.drawText(tl + QPointF(2, -2), text);

        // 选中时绘制角点
        if (isSel) {
            p.setPen(Qt::NoPen);
            for (int k = 0; k < 4; ++k) {
                const QPointF w = imageToWidget(
                    (k == 0)   ? d.p0
                    : (k == 1) ? d.p1
                    : (k == 2) ? d.p2
                               : d.p3);
                const bool hot = (k == hoverHandle_ || k == dragHandle_);
                p.setBrush(hot ? QColor(255, 200, 0) : QColor(0, 180, 255));
                p.drawEllipse(w, kHandleRadius_, kHandleRadius_);
            }
        }
    }
    p.restore();
}

void ImageCanvas::drawRoi(QPainter& p) const {
    if (roiImg_.isNull())
        return;
    const QRect rw = QRect(
                         imageToWidget(roiImg_.topLeft()).toPoint(),
                         imageToWidget(roiImg_.bottomRight()).toPoint())
                         .normalized();

    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 100));
    QPainterPath path;
    path.addRect(rect());
    QPainterPath hole;
    hole.addRect(rw);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPath(path - hole);
    p.restore();

    p.setPen(QPen(Qt::yellow, 2));
    p.drawRect(rw);
    p.setPen(Qt::white);
    p.drawText(
        rw.adjusted(4, 4, -4, -4), Qt::AlignLeft | Qt::AlignTop,
        QString("%1×%2").arg(roiImg_.width()).arg(roiImg_.height()));
}

void ImageCanvas::drawCrosshair(QPainter& p) const {
    if (!mouseInside_ || img_.isNull())
        return;
    const QRectF R = imageRectOnWidget();
    if (!R.contains(mousePosW_))
        return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setClipRect(R);
    p.setPen(QPen(QColor(0, 255, 0, 180), 1));
    p.drawLine(QPoint(mousePosW_.x(), int(R.top())), QPoint(mousePosW_.x(), int(R.bottom())));
    p.drawLine(QPoint(int(R.left()), mousePosW_.y()), QPoint(int(R.right()), mousePosW_.y()));
    p.restore();
}

/* ===== 交互 ===== */
void ImageCanvas::wheelEvent(QWheelEvent* e) {
    if (img_.isNull()) {
        e->accept();
        return;
    }
    const QPointF cursorW = e->position();
    const QPointF beforeI = widgetToImage(cursorW);
    const double step     = e->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
    const double newScale = std::clamp(scale_ * step, kMinScale_, kMaxScale_);
    scale_                = newScale;
    const QPointF afterW  = imageToWidget(beforeI);
    pan_ += (cursorW - afterW);
    update();
    e->accept();
}

void ImageCanvas::mousePressEvent(QMouseEvent* e) {
    if (img_.isNull())
        return;
    lastMousePos_ = e->pos();
    mousePosW_    = e->pos();
    mouseInside_  = rect().contains(mousePosW_);

    if (e->button() == Qt::LeftButton) {
        // 1) 若有选中，优先检测角点拖动
        if (selectedIndex_ >= 0 && selectedIndex_ < dets_.size()) {
            hoverHandle_ = hitHandleOnSelected(e->pos());
            if (hoverHandle_ >= 0) {
                dragHandle_ = hoverHandle_;
                update();
                return;
            }
        }

        // 2) 命中已有目标 → 选中，不画框
        const int hit = hitDetectionStrict(e->pos());
        if (hit >= 0) {
            if (selectedIndex_ != hit) {
                selectedIndex_ = hit;
                emit detectionSelected(selectedIndex_);
            }
            update();
            return;
        }

        // 3) 空白 → 开始画新框
        draggingRect_   = true;
        dragRectStartW_ = e->pos();
        const QPoint a  = widgetToImage(dragRectStartW_).toPoint();
        dragRectImg_    = QRect(a, a);
        update();
        return;
    } else if (e->button() == Qt::MiddleButton) {
        panning_ = true;
        setCursor(Qt::ClosedHandCursor);
    } else if (e->button() == Qt::RightButton) {
        const int hit = hitDetectionStrict(e->pos());
        if (hit >= 0) {
            removeDetection(hit);
            update();
            return;
        }
        // 未命中：啥也不做（但确保没有遗留拖拽状态）
        draggingRect_ = false;
        dragHandle_   = -1;
        hoverHandle_  = -1;
        update();
        return;
    }
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        // A. 结束画框 → 立刻新增
        if (draggingRect_) {
            draggingRect_ = false;
            if (!dragRectImg_.isNull()) {
                const QRect r = clampRectToImage(dragRectImg_.normalized());
                if (r.width() >= 2 && r.height() >= 2) {
                    Armor a;
                    a.cls = currentClass_.isEmpty() ? QStringLiteral("unknown") : currentClass_;
                    // TL, BL, BR, TR  (CCW)
                    a.p0 = QPointF(r.left(), r.top());
                    a.p1 = QPointF(r.left(), r.bottom());
                    a.p2 = QPointF(r.right(), r.bottom());
                    a.p3 = QPointF(r.right(), r.top());

                    dets_.append(a);
                    emit annotationCommitted(a);
                    emit detectionUpdated(dets_.size() - 1, a);
                    selectedIndex_ = dets_.size() - 1;
                    emit detectionSelected(selectedIndex_);
                }
            }
            dragRectImg_ = QRect();
            update();
            return;
        }

        if (dragHandle_ >= 0) {
            dragHandle_ = -1;
            if (selectedIndex_ >= 0 && selectedIndex_ < dets_.size()) {
                emit detectionUpdated(selectedIndex_, dets_[selectedIndex_]);
            }
            update();
            return;
        }

        if (draggingRoi_)
            endFreeRoi();
    }
    if (e->button() == Qt::MiddleButton && panning_) {
        panning_ = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* e) {
    mousePosW_   = e->pos();
    mouseInside_ = rect().contains(mousePosW_);

    if (panning_) {
        const QPoint d = e->pos() - lastMousePos_;
        pan_ += d;
        lastMousePos_ = e->pos();
        update();
        return;
    }

    if (draggingRect_) {
        QPoint a     = widgetToImage(dragRectStartW_).toPoint();
        QPoint b     = widgetToImage(e->pos()).toPoint();
        dragRectImg_ = QRect(a, b).normalized();
        update();
        return;
    }

    if (dragHandle_ >= 0 && selectedIndex_ >= 0 && selectedIndex_ < dets_.size()) {
        auto& A          = dets_[selectedIndex_];
        const QPointF pi = widgetToImage(e->pos());
        switch (dragHandle_) {
        case 0: A.p0 = pi; break;
        case 1: A.p1 = pi; break;
        case 2: A.p2 = pi; break;
        case 3: A.p3 = pi; break;
        }
        // 不在移动中重排，避免把当前拖拽句柄“换角”
        emit detectionUpdated(selectedIndex_, A);
        update();
        return;
    }

    // 仅选中时更新悬停角点
    if (selectedIndex_ >= 0 && selectedIndex_ < dets_.size()) {
        hoverHandle_ = hitHandleOnSelected(e->pos());
    } else {
        hoverHandle_ = -1;
    }

    // 悬停命中（最后）
    const int hitNow = hitDetectionStrict(e->pos());
    if (hitNow != hoverIndex_) {
        hoverIndex_ = hitNow;
        emit detectionHovered(hoverIndex_);
    }

    update(); // 始终刷新（十字线/轻微移动也更新）
}

void ImageCanvas::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton)
        return;
    const int hit = hitDetectionStrict(e->pos());
    if (hit >= 0) {
        setSelectedIndex(hit);
        promptEditSelectedClass();
    }
}

void ImageCanvas::keyPressEvent(QKeyEvent* e) {
    if (e->isAutoRepeat()) {
        e->ignore();
        return;
    }

    if (e->key() == Qt::Key_F2 || e->key() == Qt::Key_C) {
        promptEditSelectedClass();
        e->accept();
        return;
    }

    if (e->key() == Qt::Key_Escape) {
        // 取消任何进行中的微操作
        draggingRect_ = false;
        dragHandle_   = -1;
        hoverHandle_  = -1;
        update();
        e->accept();
        return;
    }

    QLabel::keyPressEvent(e);
}

void ImageCanvas::leaveEvent(QEvent*) {
    mouseInside_ = false;
    if (hoverIndex_ != -1) {
        hoverIndex_ = -1;
        emit detectionHovered(-1);
    }
    update();
}

void ImageCanvas::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    updateFitRect();
    update();
}

/* ===== 几何 & 命中 ===== */
void ImageCanvas::updateFitRect() {
    if (img_.isNull()) {
        fitRect_ = QRectF();
        return;
    }
    const QSizeF W = size();
    QSizeF sc      = img_.size();
    sc.scale(W, Qt::KeepAspectRatio);
    const QPointF off((W.width() - sc.width()) / 2.0, (W.height() - sc.height()) / 2.0);
    fitRect_ = QRectF(off, sc);
}
QRectF ImageCanvas::imageRectOnWidget() const {
    if (img_.isNull())
        return {};
    const QPointF c = fitRect_.center();
    const QSizeF s  = fitRect_.size() * scale_;
    QRectF r(QPointF(0, 0), s);
    r.moveCenter(c + pan_);
    return r;
}
QPointF ImageCanvas::widgetToImage(const QPointF& p) const {
    const QRectF R = imageRectOnWidget();
    if (img_.isNull() || R.isEmpty())
        return {};
    const double sx = img_.width() / R.width(), sy = img_.height() / R.height();
    QPointF pi((p.x() - R.x()) * sx, (p.y() - R.y()) * sy);
    pi.setX(std::clamp(pi.x(), 0.0, double(img_.width() - 1)));
    pi.setY(std::clamp(pi.y(), 0.0, double(img_.height() - 1)));
    return pi;
}
QPointF ImageCanvas::imageToWidget(const QPointF& p) const {
    const QRectF R = imageRectOnWidget();
    if (img_.isNull() || R.isEmpty())
        return {};
    const double sx = R.width() / img_.width(), sy = R.height() / img_.height();
    return QPointF(R.x() + p.x() * sx, R.y() + p.y() * sy);
}
QRect ImageCanvas::widgetRectToImageRect(const QRect& rw) const {
    const QPointF tl = widgetToImage(rw.topLeft());
    const QPointF br = widgetToImage(rw.bottomRight());
    QRect r          = QRect(tl.toPoint(), br.toPoint()).normalized();
    return clampRectToImage(r);
}
QRect ImageCanvas::clampRectToImage(const QRect& r) const {
    if (img_.isNull())
        return {};
    return r.intersected(QRect(0, 0, img_.width(), img_.height()));
}

// 仅在“选中目标”上测试角点命中
int ImageCanvas::hitHandleOnSelected(const QPoint& wpos) const {
    if (selectedIndex_ < 0 || selectedIndex_ >= dets_.size())
        return -1;
    const auto& A = dets_[selectedIndex_];
    const std::array<QPointF, 4> pts{A.p0, A.p1, A.p2, A.p3};
    for (int i = 0; i < 4; ++i) {
        if (QLineF(imageToWidget(pts[i]), wpos).length() <= kHandleRadius_ * 1.6)
            return i;
    }
    return -1;
}

int ImageCanvas::hitDetectionStrict(const QPoint& wpos) const {
    if (dets_.isEmpty())
        return -1;
    const QPointF w = wpos;
    for (int i = dets_.size() - 1; i >= 0; --i) { // 逆序：前景优先
        const auto& d = dets_[i];
        QPolygonF poly;
        poly << imageToWidget(d.p0) << imageToWidget(d.p1) << imageToWidget(d.p2)
             << imageToWidget(d.p3);
        if (pointInsidePolyW(poly, w))
            return i;
    }
    return -1;
}
bool ImageCanvas::pointInsidePolyW(const QPolygonF& polyW, const QPointF& w) const {
    return polyW.containsPoint(w, Qt::WindingFill);
}

/* ===== ROI 交互 ===== */
void ImageCanvas::beginFreeRoi(const QPoint& wpos) {
    draggingRoi_ = true;
    dragStartW_  = wpos;
    roiImg_      = QRect();
}
void ImageCanvas::updateFreeRoi(const QPoint& wpos) {
    QRect rw = QRect(dragStartW_, wpos).normalized();
    roiImg_  = widgetRectToImageRect(rw);
    emit roiChanged(roiImg_);
    update();
}
void ImageCanvas::endFreeRoi() {
    draggingRoi_ = false;
    if (!roiImg_.isNull())
        emit roiCommitted(roiImg_);
    update();
}
void ImageCanvas::placeFixedRoiAt(const QPoint& wpos) {
    if (!modelInputSize_.isValid())
        return;
    const QPointF cI = widgetToImage(wpos);
    QRect r(
        QPoint(
            int(cI.x() - modelInputSize_.width() / 2.0),
            int(cI.y() - modelInputSize_.height() / 2.0)),
        modelInputSize_);
    roiImg_ = clampRectToImage(r);
    emit roiChanged(roiImg_);
}

/* ===== UI 帮助 ===== */
void ImageCanvas::promptEditSelectedClass() {
    if (selectedIndex_ < 0 || selectedIndex_ >= dets_.size())
        return;
    const QString oldCls = dets_[selectedIndex_].cls;
    bool ok              = false;
    const QString cls    = QInputDialog::getText(
        this, tr("Edit Class"), tr("Class label:"), QLineEdit::Normal, oldCls, &ok);
    if (ok)
        setSelectedClass(cls.trimmed());
}

void ImageCanvas::setupSvg() {
    auto icons_dir  = controller::AppSettings::instance().assetsDir() + "/icons";
    svgCache_["1"]  = new QSvgRenderer(icons_dir + "/1.svg", this);
    svgCache_["2"]  = new QSvgRenderer(icons_dir + "/2.svg", this);
    svgCache_["3"]  = new QSvgRenderer(icons_dir + "/3.svg", this);
    svgCache_["4"]  = new QSvgRenderer(icons_dir + "/4.svg", this);
    svgCache_["5"]  = new QSvgRenderer(icons_dir + "/5.svg", this);
    svgCache_["Bb"] = new QSvgRenderer(icons_dir + "/Bb.svg", this);
    svgCache_["Bs"] = new QSvgRenderer(icons_dir + "/Bs.svg", this);
    svgCache_["G"]  = new QSvgRenderer(icons_dir + "/G.svg", this);
    svgCache_["O"]  = new QSvgRenderer(icons_dir + "/O.svg", this);
    qInfo() << "SVG loaded.";
}
static bool isBigType(const QString& t) {
    // 示例规则：1 / Bb / B3 / B4 / B5 按“大装甲”；其余按“小装甲”
    return (t == "1" || t == "Bb" || t == "B3" || t == "B4" || t == "B5");
}

// 拆分类别：首字母颜色(B/R/G/P)，后缀是图案类型（用来选 svg）
static void splitClass(const QString& cls, QString& color, QString& type) {
    if (!cls.isEmpty() && QStringLiteral("BRGP").contains(cls.at(0))) {
        color = cls.left(1);
        type  = cls.mid(1);
    } else {
        color.clear();
        type = cls;
    }
}

void ImageCanvas::drawSvg(QPainter& p, const QVector<Armor>& armors) const {
    if (armors.isEmpty())
        return;

    p.save();

    // ---- 1) 预备：两套 SVG 外框四角（viewBox）和两套“锚点”（全是 SVG 坐标，顺序 TL, BL, BR, TR）
    QPolygonF big_svg_quad, small_svg_quad;
    big_svg_quad << QPointF(0., 0.) << QPointF(0., 478.) << QPointF(871., 478.)
                 << QPointF(871., 0.);
    small_svg_quad << QPointF(0., 0.) << QPointF(0., 516.) << QPointF(557., 516.)
                   << QPointF(557., 0.);

    QPolygonF big_anchors, small_anchors; // TL, BL, BR, TR（单位：SVG）
    big_anchors << QPointF(0., 140.61) << QPointF(0., 347.39) << QPointF(871., 347.39)
                << QPointF(871., 140.61);
    small_anchors << QPointF(0., 143.26) << QPointF(0., 372.74) << QPointF(557., 372.74)
                  << QPointF(557., 143.26);

    // 画布（控件）四角（顺序也用 TL, BL, BR, TR）
    QPolygonF painter_quad;
    painter_quad << QPointF(0., 0.) << QPointF(0., height()) << QPointF(width(), height())
                 << QPointF(width(), 0.);

    // 先把 SVG 外框四角 -> 画布四角，得到“把 SVG 坐标投到画布坐标”的仿射/投影
    QTransform big_svg2painter, small_svg2painter;
    QTransform::quadToQuad(big_svg_quad, painter_quad, big_svg2painter);
    QTransform::quadToQuad(small_svg_quad, painter_quad, small_svg2painter);

    // 把“SVG 的锚点”变到画布坐标（作为 quadToQuad 的 src）
    const QPolygonF big_src_on_painter   = big_svg2painter.map(big_anchors);
    const QPolygonF small_src_on_painter = small_svg2painter.map(small_anchors);

    for (const auto& a : armors) {
        // —— 解析类别：取颜色 & 图案类型（用类型去找 svg）
        QString color, type;
        splitClass(a.cls, color, type);

        // 找到对应的 QSvgRenderer（建议你的 svgCache_ 用“类型名”做 key，比如
        // "1","2","Bb","Bs","S","O"...）
        auto it = svgCache_.find(type);
        if (it == svgCache_.end() || it.value() == nullptr) {
            qWarning() << "SVG not found for type" << type;
            continue;
        }
        QSvgRenderer* renderer = it.value();
        if (!renderer->isValid())
            continue;

        // —— 目标四点（画布坐标）；注意 Armor 的顺序：p0=TL, p1=BL, p2=BR, p3=TR
        QPolygonF dst;
        dst << imageToWidget(a.p0)  // TL
            << imageToWidget(a.p1)  // BL
            << imageToWidget(a.p2)  // BR
            << imageToWidget(a.p3); // TR

        // 选择 big/small 的“源四点”（同样是 TL, BL, BR, TR；已在画布坐标）
        const QPolygonF& src = isBigType(type) ? big_src_on_painter : small_src_on_painter;

        // —— 求单应 & 渲染
        QTransform H;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (!QTransform::quadToQuad(src, dst, H))
            continue;
#else
        if (!QTransform::quadToQuad(src, dst, H))
            continue;
#endif
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setTransform(H, true);
        renderer->render(&p); // SVG 将按“锚点→目标四点”透视过去
        p.restore();

        // （可选）根据 color 设置边框主色等，你可以在别处画：B/R/G/P → 蓝/红/灰/紫
        // 例如：if (color=="B") pen = blue; ...
    }

    p.restore();
}
