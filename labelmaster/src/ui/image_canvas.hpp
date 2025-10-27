#pragma once
#include <QLabel>
#include <QImage>
#include <QRect>
#include <QVector>
#include <QString>

class QPainter;

class ImageCanvas : public QLabel {
    Q_OBJECT
public:
    struct Armor {
        QString cls;          // 类别名
        float   score = 0.f;  // 置信度 [0,1]
        QPointF p0, p1, p2, p3; // 四角点（原图坐标系，顺序任意，闭合绘制）
    };

    enum class RoiMode { Free, FixedToModelSize };

    explicit ImageCanvas(QWidget* parent = nullptr);

    // 图像输入
    bool    loadImage(const QString& path);
    void    setImage(const QImage& img);
    const QImage& currentImage() const { return img_; }
    QString currentImagePath() const { return imgPath_; }

    // 模型输入大小与 ROI 模式
    void setModelInputSize(const QSize& s);          // 例如 640x480
    void setRoiMode(RoiMode m);                      // Free 或 FixedToModelSize
    RoiMode roiMode() const { return roiMode_; }

    // ROI（原图坐标）
    QRect roi() const { return roiImg_; }
    void  clearRoi();
    QImage cropRoi() const;                          // 裁剪 ROI（若无 ROI 返回空）

    // 视图控制
    void   resetView();
    double scaleFactor() const { return scale_; }

    // 检测请求（发整图或 ROI）
    Q_SLOT void requestDetectFull();                 // emit detectRequested(img_)
    Q_SLOT void requestDetectRoi();                  // emit detectRequested(cropRoi())

    // 检测结果显示
    Q_SLOT void setDetections(const QVector<Armor>& dets);
    Q_SLOT void clearDetections();

signals:
    void roiChanged(const QRect& roiImg);            // ROI 变化时（原图坐标）
    void roiCommitted(const QRect& roiImg);          // 松开鼠标时“确认”的 ROI
    void detectRequested(const QImage& image);       // 抛给外部 detector（整图或 ROI）

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    // —— 几何映射（原图 <-> 组件）——
    void   updateFitRect();
    QRectF imageRectOnWidget() const;
    QPointF widgetToImage(const QPointF& p) const;
    QPointF imageToWidget(const QPointF& p) const;
    QRect  widgetRectToImageRect(const QRect& rw) const;
    QRect  clampRectToImage(const QRect& r) const;

    // —— 绘制 —— 
    void drawCrosshair(QPainter& p) const;
    void drawRoi(QPainter& p) const;
    void drawDetections(QPainter& p) const;

    // —— ROI 交互 —— 
    void beginFreeRoi(const QPoint& wpos);
    void updateFreeRoi(const QPoint& wpos);
    void endFreeRoi();

    void placeFixedRoiAt(const QPoint& wpos); // 将固定尺寸 ROI 的中心放到鼠标处（原图约束）

private:
    // 图像
    QImage  img_;
    QString imgPath_;

    // 视图
    double  scale_ = 1.0;
    QPointF pan_{0, 0};
    QRectF  fitRect_;        // 适配控件等比后的矩形（widget 坐标）

    // 鼠标状态
    QPoint  lastMousePos_;
    bool    panning_ = false;
    bool    mouseInside_ = false;
    QPoint  mousePosW_{-1, -1};

    // ROI（原图坐标）
    RoiMode roiMode_ = RoiMode::Free;
    QSize   modelInputSize_;
    QRect   roiImg_;
    bool    draggingRoi_ = false; // 仅 Free 模式使用
    QPoint  dragStartW_;

    // 检测结果
    QVector<Armor> dets_;

    // 参数
    const double kMinScale_ = 0.2;
    const double kMaxScale_ = 8.0;
};
