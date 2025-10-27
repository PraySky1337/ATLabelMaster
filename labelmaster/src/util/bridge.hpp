#pragma once
#include <opencv2/imgproc.hpp>
#include <QImage>

cv::Mat qimageToMat(const QImage& img) {
    if (img.isNull()) return {};
    QImage converted = img.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(converted.height(), converted.width(), CV_8UC3,
                const_cast<uchar*>(converted.bits()), converted.bytesPerLine());
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGB2BGR);
    return bgr.clone();
}

QImage matToQImage(const cv::Mat& m) {
    if (m.empty()) return {};
    cv::Mat rgb;
    if (m.type() == CV_8UC3) {
        cv::cvtColor(m, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    } else if (m.type() == CV_8UC4) {
        // 假定 BGRA
        cv::cvtColor(m, rgb, cv::COLOR_BGRA2RGBA);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGBA8888).copy();
    } else if (m.type() == CV_8UC1) {
        return QImage(m.data, m.cols, m.rows, m.step, QImage::Format_Grayscale8).copy();
    } else {
        cv::Mat tmp;
        m.convertTo(tmp, CV_8UC3);
        cv::cvtColor(tmp, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
}
