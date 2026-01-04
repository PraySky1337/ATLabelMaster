#include "controller/settings.hpp"
#include "detector/smart_detector.hpp"
#include "logger/core.hpp"
#include "service/file.hpp"
#include "ui/image_canvas.hpp"
#include "ui/info_dialog.h"
#include "ui/mainwindow.hpp"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <pthread.h>
#include <qapplication.h>
#include <qcoreapplication.h>
#include <qglobal.h>
#include <qobject.h>
#include <qtmetamacros.h>

int main(int argc, char* argv[]) {
    // 1) 初始化应用配置系统(在 QApplication 之前，否则会Unknown Organization)
    // 2) 安装 Qt 的全局消息处理器，尽早捕获日志
    QApplication app(argc, argv);
    controller::AppSettings::initOrgApp("LabelMaster", "LabelMaster", "LabelMaster.org");
#ifdef Q_OS_MAC
    // 在 Mac 上，从 MacOS 目录往上退一级，再进入 Resources
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    controller::AppSettings::instance().setassetsDir(dir.absolutePath() + "/Resources");
#else
    controller::AppSettings::instance().setassetsDir(
        QCoreApplication::applicationDirPath() + "/assets");
#endif
    logger::Logger::installQtHandler();
    ui::MainWindow w;
    FileService files;
    rm_auto_aim::Detector::LightParams lp;
    rm_auto_aim::Detector::ArmorParams ap;
    SmartDetector detector{&w};
    // if (QFile::exists(assets_dir)) {
    //     QString model_path = assets_dir + "/models/mlp.onnx";
    //     QString label_path = assets_dir + "/models/label.txt";
    //     controller::AppSettings::instance();
    //     detector.resetNumberClassifier(
    //         model_path, label_path,
    //         controller::AppSettings::instance().numberClassifierThreshold());
    // } else {
    //     qWarning() << "Assets directory not found: " << assets_dir;
    // }

    // MainWindow <-> FileService 其他连接保持
    QObject::connect(&w, &ui::MainWindow::sigOpenFolderRequested, &files, [&]() {
        files.openFolderDialog(DataSet::LabelMaster);
    });
    QObject::connect(
        &w, &ui::MainWindow::sigImportFolderRequested, &files, &FileService::importFrom);
    QObject::connect(&w, &ui::MainWindow::sigFileActivated, &files, &FileService::openIndex);
    QObject::connect(&w, &ui::MainWindow::sigDroppedPaths, &files, &FileService::openPaths);
    QObject::connect(&w, &ui::MainWindow::sigNextRequested, &files, [&]() { files.next(); });
    QObject::connect(&w, &ui::MainWindow::sigPrevRequested, &files, &FileService::prev);
    QObject::connect(&w, &ui::MainWindow::sigDeleteRequested, &files, &FileService::deleteCurrent);
    QObject::connect(&w, &ui::MainWindow::sigGetStasRequested, &files, &FileService::getStas);
    QObject::connect(
        &w, &ui::MainWindow::sigSettingsRequested, &w, &ui::MainWindow::showSettingDialog);
    QObject::connect(&files, &FileService::modelReady, &w, &ui::MainWindow::setFileModel);
    QObject::connect(&files, &FileService::rootChanged, &w, &ui::MainWindow::setRoot); // ★ 新增
    QObject::connect(
        &files, &FileService::currentIndexChanged, &w, &ui::MainWindow::setCurrentIndex);
    QObject::connect(&files, &FileService::imageReady, &w, &ui::MainWindow::showImage);
    QObject::connect(&files, &FileService::status, &w, &ui::MainWindow::setStatus);
    QObject::connect(&files, &FileService::busy, &w, &ui::MainWindow::setBusy);
    QObject::connect(&files, &FileService::StasGetted, &w, &ui::MainWindow::sigStasUpdateRequested);
    QObject::connect(&files, &FileService::saveRequested, &w, &ui::MainWindow::sigSaveRequested);
    QObject::connect(
        &w, &ui::MainWindow::sigHistEqRequested, w.ui()->label, &ImageCanvas::histEqualize);
    // ImageCanvas <-> SmartDetector 连接 检测和检测结果
    QObject::connect(
        w.ui()->label, &ImageCanvas::detectRequested, &detector, &SmartDetector::detect);
    QObject::connect(
        &detector, &SmartDetector::detected, w.ui()->label, &ImageCanvas::setDetections);
    QObject::connect(
        &files, &FileService::labelsLoaded, w.ui()->label, &ImageCanvas::setDetections);
    QObject::connect(
        w.ui()->label, &ImageCanvas::annotationsPublished, &files, &FileService::saveData);
    files.exposeModel();
    w.enableDragDrop(true);
    w.show();
    LOGI("App started");
    return app.exec();
}
