#include "smart_detector.hpp"
#include "detector/traditional/number_classifier.hpp"
#include "util/bridge.hpp"

#include <QDebug>
#include <QMetaType>
#include <QtGlobal>
#include <memory>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <qglobal.h>

using rm_auto_aim::Detector;

SmartDetector::SmartDetector(
    int bin_thres, const Detector::LightParams& lp, const Detector::ArmorParams& ap,
    QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<std::vector<rm_auto_aim::Armor>>("std::vector<rm_auto_aim::Armor>");
    detector_ = std::make_unique<Detector>(bin_thres, lp, ap);
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
    qInfo() << "detect once";
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
            cv::imshow("test", mat);
            cv::waitKey(1);
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
        QVector<::Armor> sigArmors;
        for (const auto& armor : armors) {
            ::Armor sigArmor;
            sigArmor.color = armor.left_light.color == 0 ? "red" : "blue";
            sigArmor.p0    = QPointF(armor.left_light.top.x, armor.left_light.top.y);
            sigArmor.p1    = QPointF(armor.left_light.bottom.x, armor.left_light.bottom.y);
            sigArmor.p2    = QPointF(armor.right_light.bottom.x, armor.right_light.bottom.y);
            sigArmor.p3    = QPointF(armor.right_light.top.x, armor.right_light.top.y);
            sigArmor.cls   = QString().fromStdString(armor.number);
            sigArmors.emplace_back(sigArmor);
        }

        // 调试图像（可选）
        QImage bin   = matToQImage(detector_->binary_img);
        cv::Mat draw = input.clone();
        detector_->drawResults(draw);
        QImage anno = matToQImage(draw);

        qDebug() << "emit detected";
        emit detected(sigArmors);
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

void SmartDetector::resetNumberClassifier(
    const QString& model_path, const QString& label_path, float threshold) {
    if (detector_) {
        detector_->classifier.reset();
        detector_->classifier = std::make_unique<rm_auto_aim::NumberClassifier>(
            model_path.toStdString(), label_path.toStdString(), threshold);
    } else {
        qWarning() << "SmartDetector not initialized.";
    }
}