#include "smart_detector.hpp"
#include "detector/traditional/number_classifier.hpp"
#include "util/bridge.hpp"

#include <QMetaType>
#include <QtGlobal>
#include <memory>
#include <opencv2/imgproc.hpp>

using rm_auto_aim::Detector;

SmartDetector::SmartDetector(
    int bin_thres, const Detector::LightParams& lp, const Detector::ArmorParams& ap,
    QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<std::vector<rm_auto_aim::Armor>>("std::vector<rm_auto_aim::Armor>");
    detector_             = std::make_unique<Detector>(bin_thres, lp, ap);
    detector_->classifier = std::make_unique<rm_auto_aim::NumberClassifier>(
        "/home/developer/ws/model/mlp.onnx", "/home/developer/ws/model/label.txt", 50.0);
}

void SmartDetector::setBinaryThreshold(int thres) {
    if (detector_)
        detector_->binary_thres = thres;
}

void SmartDetector::detect(const QImage& image) {
    try {
        cv::Mat mat = qimageToMat(image);
        detectMat(mat);
    } catch (const std::exception& e) {
        emit error(QString("SmartDetector::detect(QImage) error: %1").arg(e.what()));
    }
}

void SmartDetector::detectMat(const cv::Mat& mat) {
    try {
        if (!detector_) {
            emit error("SmartDetector not initialized.");
            return;
        }

        cv::Mat input;
        // 统一转为 BGR 8UC3（取决于你 detector 的预期，这里假定 BGR）
        if (mat.empty()) {
            emit error("Input Mat is empty.");
            return;
        }
        if (mat.type() == CV_8UC3) {
            input = mat.clone();
            // 如果是 RGB，可在这里 swap：cv::cvtColor(mat, input, cv::COLOR_RGB2BGR);
        } else if (mat.type() == CV_8UC4) {
            cv::cvtColor(mat, input, cv::COLOR_BGRA2BGR);
        } else if (mat.type() == CV_8UC1) {
            cv::cvtColor(mat, input, cv::COLOR_GRAY2BGR);
        } else {
            mat.convertTo(input, CV_8UC3);
        }


        // --- 同步版本 ---
        auto armors = detector_->detect(input);

        // 调试图像（可选）
        QImage bin   = matToQImage(detector_->binary_img);
        cv::Mat draw = input.clone();
        detector_->drawResults(draw);
        QImage anno = matToQImage(draw);

        emit detected(armors);
        emit debugImages(bin, anno);

        // --- 异步版本（可选）---
        // QtConcurrent::run([this, input]() {
        //     auto armors = detector_->detect(input);
        //     QImage bin = matToQImage(detector_->binary_img);
        //     cv::Mat draw = input.clone();
        //     detector_->drawResults(draw);
        //     QImage anno = matToQImage(draw);
        //     emit detected(armors);
        //     emit debugImages(bin, anno);
        // });
    } catch (const std::exception& e) {
        emit error(QString("SmartDetector::detectMat error: %1").arg(e.what()));
    }
}
