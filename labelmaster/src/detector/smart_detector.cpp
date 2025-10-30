#include "smart_detector.hpp"
#include "controller/settings.hpp"
#include "util/bridge.hpp"

#include <QDebug>
#include <QMetaType>
#include <QtGlobal>
#include <memory>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <qglobal.h>
#include <qobject.h>

using rm_auto_aim::Detector;

SmartDetector::SmartDetector(
    int bin_thres, const Detector::LightParams& lp, const Detector::ArmorParams& ap,
    QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<std::vector<rm_auto_aim::Armor>>("std::vector<rm_auto_aim::Armor>");
    traditional_detector_ = std::make_unique<Detector>(bin_thres, lp, ap);
    mode                  = Mode::Traditional;
}

SmartDetector::SmartDetector(QObject* parent)
    : QObject(parent) {
    ai_detector_ = std::make_unique<ai::Detector>();
    ai_detector_->setupModel(controller::AppSettings::instance().assetsDir());
    mode = Mode::AI;
}

void SmartDetector::setBinaryThreshold(int thres) {
    if (traditional_detector_)
        traditional_detector_->binary_thres = thres;
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
        QVector<::Armor> sigArmors;
        if (ai_detector_) {
            sigArmors = ai_detector_->detect(input);
        } else {
            qWarning() << "ai detector not initialized.";
        }

        // 调试图像（可选）
        cv::Mat draw = input.clone();
        QImage anno  = matToQImage(draw);

        qDebug() << "emit detected";
        emit detected(sigArmors);

    } catch (const std::exception& e) {
        emit error(QString("SmartDetector::detectMat error: %1").arg(e.what()));
    }
}

void SmartDetector::resetNumberClassifier(
    const QString& model_path, const QString& label_path, float threshold) {
    if (traditional_detector_) {
        traditional_detector_->classifier.reset();
        traditional_detector_->classifier = std::make_unique<rm_auto_aim::NumberClassifier>(
            model_path.toStdString(), label_path.toStdString(), threshold);
    } else {
        qWarning() << "traditional detector not initialized.";
    }
}