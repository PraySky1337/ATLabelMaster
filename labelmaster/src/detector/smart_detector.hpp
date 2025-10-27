#pragma once
#include <QObject>
#include <QImage>
#include <memory>
#include <vector>

#include <opencv2/core.hpp>
#include "traditional/detector.hpp"   // 你给的头
#include "armor.hpp"      // rm_auto_aim::Armor

// 声明给 Qt 的元类型（用于跨线程信号）
Q_DECLARE_METATYPE(std::vector<rm_auto_aim::Armor>)

class SmartDetector : public QObject {
    Q_OBJECT
public:
    explicit SmartDetector(
        int bin_thres,
        const rm_auto_aim::Detector::LightParams& lp,
        const rm_auto_aim::Detector::ArmorParams& ap,
        QObject* parent = nullptr);

    // 若需动态改参数，可暴露这两个接口（可选）
    void setBinaryThreshold(int thres);

signals:
    // 主结果：一帧检测出的装甲板
    void detected(const std::vector<rm_auto_aim::Armor>& armors);
    // 可选调试输出：二值图与标注图（若不用可删）
    void debugImages(const QImage& bin, const QImage& annotated);
    // 出错时
    void error(const QString& message);

public slots:
    // 传入 QImage
    void detect(const QImage& image);
    // 传入 cv::Mat（BGR/RGB 都可，见实现）
    void detectMat(const cv::Mat& mat);

private:


private:
    std::unique_ptr<rm_auto_aim::Detector> detector_;
};
