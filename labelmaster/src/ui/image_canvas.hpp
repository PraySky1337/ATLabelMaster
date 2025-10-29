#pragma once
#include <QImage>
#include <QLabel>
#include <QPolygonF>
#include <QRect>
#include <QString>
#include <QVector>

class QPainter;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class ImageCanvas : public QLabel {
    Q_OBJECT
public:
    struct Armor {
        QString cls;
        float score = 0.f;
        // 角点顺序：从 0 开始逆时针：TL(0) → BL(1) → BR(2) → TR(3)，全为“原图坐标”
        QPointF p0, p1, p2, p3;
    };

    enum class RoiMode { Free, FixedToModelSize };

    explicit ImageCanvas(QWidget* parent = nullptr);

    // 图像与 ROI
    bool loadImage(const QString& path);
    void setImage(const QImage& img);
    const QImage& currentImage() const { return img_; }
    QString currentImagePath() const { return imgPath_; }

    void setModelInputSize(const QSize& s);
    void setRoiMode(RoiMode m);
    RoiMode roiMode() const { return roiMode_; }
    QRect roi() const { return roiImg_; }
    void clearRoi();
    QImage cropRoi() const;

    // 视图
    void resetView();
    double scaleFactor() const { return scale_; }

public slots:
    // 检测请求
    void requestDetectFull();
    void requestDetectRoi();

    // 检测结果显示/外部读写
    void setDetections(const QVector<Armor>& dets);  // 覆盖全部
    void clearDetections();
    void addDetection(const Armor& a);               // 追加一个
    void updateDetection(int index, const Armor& a); // 更新一个
    void removeDetection(int index);                 // 删除一个

    // 类别与选中
    void setCurrentClass(const QString& cls) { currentClass_ = cls; } // 新框默认
    QString currentClass() const { return currentClass_; }
    bool setSelectedClass(const QString& cls);                        // 改“选中框”的 cls
    bool setSelectedIndex(int idx);                                   // -1 取消选中
    int selectedIndex() const { return selectedIndex_; }

    // 批量发布（图像路径、ROI、全部框）
    void publishAnnotations();

    // 导出/导入 label（JSON）
    bool saveLabelsToJsonFile(const QString& path) const;
    bool loadLabelsFromJsonFile(const QString& path);
    // 工具：保持角点顺序为 TL,BL,BR,TR（CCW）
    static void normalizeArmorCCW(Armor& a);

signals:
    // ROI
    void roiChanged(const QRect& roiImg);
    void roiCommitted(const QRect& roiImg);

    // 检测请求
    void detectRequested(const QImage& image);

    // 新框提交（松手即提交）
    void annotationCommitted(const ImageCanvas::Armor&);

    // 选中/悬停/更新/删除
    void detectionSelected(int index);                           // -1 无选中
    void detectionHovered(int index);                            // -1 无悬停
    void detectionUpdated(int index, const ImageCanvas::Armor&); // 类别或点被改
    void detectionRemoved(int index);                            // 删除哪个

    // 批量发布（供外部保存/网络发送）
    void annotationsPublished(
        const QString& imagePath, const QRect& roiImg, const QVector<ImageCanvas::Armor>& dets);

protected:
    // 绘制与交互
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void leaveEvent(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    // 命中 & 几何
    int hitHandleOnSelected(const QPoint& wpos) const; // 命中当前“选中目标”的角点
    int hitDetectionStrict(const QPoint& wpos) const;  // 严格在框内才算命中
    bool pointInsidePolyW(const QPolygonF& polyW, const QPointF& w) const;
    void drawDragRect(QPainter& p) const;              // 拖框预览

    void promptEditSelectedClass();

    void updateFitRect();
    QRectF imageRectOnWidget() const;
    QPointF widgetToImage(const QPointF& p) const;
    QPointF imageToWidget(const QPointF& p) const;
    QRect widgetRectToImageRect(const QRect& rw) const;
    QRect clampRectToImage(const QRect& r) const;

    // 绘制
    void drawCrosshair(QPainter& p) const;
    void drawRoi(QPainter& p) const;
    void drawDetections(QPainter& p) const; // 高亮选中/悬停 + 选中显示角点

    // ROI 交互
    void beginFreeRoi(const QPoint& wpos);
    void updateFreeRoi(const QPoint& wpos);
    void endFreeRoi();
    void placeFixedRoiAt(const QPoint& wpos);

private:
    // 图像
    QImage img_;
    QString imgPath_;

    // 视图
    double scale_ = 1.0;
    QPointF pan_{0, 0};
    QRectF fitRect_;

    // 鼠标
    QPoint lastMousePos_;
    bool panning_     = false;
    bool mouseInside_ = false;
    QPoint mousePosW_{-1, -1};

    // ROI
    RoiMode roiMode_ = RoiMode::Free;
    QSize modelInputSize_;
    QRect roiImg_;
    bool draggingRoi_ = false;
    QPoint dragStartW_;

    // 检测结果
    QVector<Armor> dets_;
    int selectedIndex_ = -1;
    int hoverIndex_    = -1;

    // 新增/编辑状态（正常状态内的细分）
    bool draggingRect_ = false; // 正在画新框
    QPoint dragRectStartW_;
    QRect dragRectImg_;

    int dragHandle_  = -1;      // 正在拖动的角点（仅对 selected 生效）
    int hoverHandle_ = -1;      // 悬停角点（仅对 selected 生效）

    QString currentClass_;

    // 参数
    const double kMinScale_  = 0.2;
    const double kMaxScale_  = 8.0;
    const int kHandleRadius_ = 6; // 角点渲染半径（像素，屏幕坐标）

};

Q_DECLARE_METATYPE(ImageCanvas::Armor)
Q_DECLARE_METATYPE(QVector<ImageCanvas::Armor>)
