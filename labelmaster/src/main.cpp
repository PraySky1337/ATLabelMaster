#include "logger/core.hpp"
#include "service/file.hpp"
#include "controller/settings.hpp"
#include "ui/mainwindow.hpp"
#include <QApplication>
#include <pthread.h>

#define ASSETS_PATH "/home/developer/ws/assets"

int main(int argc, char* argv[]) {
    // 1) 先安装 Qt 的全局消息处理器，尽早捕获日志
    QApplication app(argc, argv);

    logger::Logger::installQtHandler();
    ui::MainWindow w;
    FileService files;
    controller::AppSettings::instance().setAssetsDir(ASSETS_PATH);
    
    
    // MainWindow <-> FileService 其他连接保持
    QObject::connect(
        &w, &ui::MainWindow::sigOpenFolderRequested, &files, &FileService::openFolderDialog);
    QObject::connect(&w, &ui::MainWindow::sigFileActivated, &files, &FileService::openIndex);
    QObject::connect(&w, &ui::MainWindow::sigDroppedPaths, &files, &FileService::openPaths);
    QObject::connect(&w, &ui::MainWindow::sigNextRequested, &files, &FileService::next);
    QObject::connect(&w, &ui::MainWindow::sigPrevRequested, &files, &FileService::prev);
    QObject::connect(&w, &ui::MainWindow::sigDeleteRequested, &files, &FileService::deleteCurrent);

    // ↓↓↓ 不再需要 files -> appendLog 的连接（交由全局 Logger 输出）

    QObject::connect(&files, &FileService::modelReady, &w, &ui::MainWindow::setFileModel);
    QObject::connect(&files, &FileService::rootChanged, &w, &ui::MainWindow::setRoot); // ★ 新增
    QObject::connect(
        &files, &FileService::currentIndexChanged, &w, &ui::MainWindow::setCurrentIndex);
    QObject::connect(&files, &FileService::imageReady, &w, &ui::MainWindow::showImage);
    QObject::connect(&files, &FileService::status, &w, &ui::MainWindow::setStatus);
    QObject::connect(&files, &FileService::busy, &w, &ui::MainWindow::setBusy);

    files.exposeModel();
    w.enableDragDrop(true);
    w.show();

    LOGI("App started");
    return app.exec();
}
