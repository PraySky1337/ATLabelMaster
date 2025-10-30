#pragma once
#include "ai/detector.hpp"
#include <QImage>
#include <QObject>
#include <QVector>
#include <memory>

#include "armor.hpp"                // rm_auto_aim::Armor
#include "traditional/detector.hpp" // 你给的头
#include "types.hpp"
#include <opencv2/core.hpp>

// 声明给 Qt 的元类型（用于跨线程信号）
Q_DECLARE_METATYPE(std::vector<rm_auto_aim::Armor>)

class SmartDetector : public QObject {
    Q_OBJECT
public:
    enum Mode { Traditional, AI };
    explicit SmartDetector(
        int bin_thres, const rm_auto_aim::Detector::LightParams& lp,
        const rm_auto_aim::Detector::ArmorParams& ap, QObject* parent = nullptr);
    explicit SmartDetector(QObject* parent = nullptr);

    void setBinaryThreshold(int thres);

signals:
    // 主结果：一帧检测出的装甲板
    void detected(const QVector<Armor>& armors);
    // 可选调试输出：二值图与标注图（若不用可删）
    void debugImages(const QImage& bin, const QImage& annotated);
    // 出错时
    void error(const QString& message);

public slots:
    // 传入 QImage
    void detect(const QImage& image);
    // 传入 cv::Mat（BGR/RGB 都可，见实现）
    void detectMat(const cv::Mat& mat);
    // 重置分类器
    void resetNumberClassifier(
        const QString& model_path, const QString& label_path, float threshold);

private:
    Mode mode = Mode::AI;
    std::unique_ptr<rm_auto_aim::Detector> traditional_detector_;
    std::unique_ptr<ai::Detector> ai_detector_;
};
