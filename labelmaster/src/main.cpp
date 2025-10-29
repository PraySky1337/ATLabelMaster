#include "controller/settings.hpp"
#include "detector/smart_detector.hpp"
#include "logger/core.hpp"
#include "service/file.hpp"
#include "ui/image_canvas.hpp"
#include "ui/mainwindow.hpp"
#include <QApplication>
#include <QFile>
#include <pthread.h>
#include <qglobal.h>

#define ASSETS_PATH "/home/developer/ws/assets"

int main(int argc, char* argv[]) {
    // 1) 先安装 Qt 的全局消息处理器，尽早捕获日志
    QApplication app(argc, argv);

    logger::Logger::installQtHandler();
    ui::MainWindow w;
    FileService files;
    rm_auto_aim::Detector::LightParams lp;
    rm_auto_aim::Detector::ArmorParams ap;
    SmartDetector detector{90, lp, ap, &w};
    auto assets_dir = controller::AppSettings::instance().assetsDir();
    if (QFile::exists(assets_dir)) {
        QString model_path = assets_dir + "/models/mlp.onnx";
        QString label_path = assets_dir + "/models/label.txt";
        controller::AppSettings::instance();
        detector.resetNumberClassifier(
            model_path, label_path,
            controller::AppSettings::instance().numberClassifierThreshold());
    } else {
        qWarning() << "Assets directory not found: " << assets_dir;
    }
    

    // MainWindow <-> FileService 其他连接保持
    QObject::connect(
        &w, &ui::MainWindow::sigOpenFolderRequested, &files, &FileService::openFolderDialog);
    QObject::connect(&w, &ui::MainWindow::sigFileActivated, &files, &FileService::openIndex);
    QObject::connect(&w, &ui::MainWindow::sigDroppedPaths, &files, &FileService::openPaths);
    QObject::connect(&w, &ui::MainWindow::sigNextRequested, &files, &FileService::next);
    QObject::connect(&w, &ui::MainWindow::sigPrevRequested, &files, &FileService::prev);
    QObject::connect(&w, &ui::MainWindow::sigDeleteRequested, &files, &FileService::deleteCurrent);

    QObject::connect(&files, &FileService::modelReady, &w, &ui::MainWindow::setFileModel);
    QObject::connect(&files, &FileService::rootChanged, &w, &ui::MainWindow::setRoot); // ★ 新增
    QObject::connect(
        &files, &FileService::currentIndexChanged, &w, &ui::MainWindow::setCurrentIndex);
    QObject::connect(&files, &FileService::imageReady, &w, &ui::MainWindow::showImage);
    QObject::connect(&files, &FileService::status, &w, &ui::MainWindow::setStatus);
    QObject::connect(&files, &FileService::busy, &w, &ui::MainWindow::setBusy);

    // ImageCanvas <-> SmartDetector 连接 检测和检测结果
    QObject::connect(w.ui()->label, &ImageCanvas::detectRequested, &detector, &SmartDetector::detect);
    QObject::connect(&detector, &SmartDetector::detected, w.ui()->label, &ImageCanvas::setDetections);
    files.exposeModel();
    w.enableDragDrop(true);
    w.show();

    LOGI("App started");
    return app.exec();
}
