/**
 * @file application_wiring.cpp
 * @brief Implementation of application wiring
 */

#include "application_wiring.hpp"
#include "ui/mainwindow.hpp"
#include "service/file.hpp"
#include "detector/smart_detector.hpp"
#include "ui/image_canvas.hpp"

namespace labelmaster::core {

ApplicationWiring::ApplicationWiring(
    ::ui::MainWindow& mainWindow,
    FileService& fileService,
    SmartDetector& detector,
    QObject* parent)
    : QObject(parent)
    , mainWindow_(mainWindow)
    , fileService_(fileService)
    , detector_(detector) {
}

void ApplicationWiring::wire() {
    connectMainWindowToFileService();
    connectFileServiceToMainWindow();
    connectMainWindowInternal();
    connectImageCanvasToDetector();
    connectDetectorToImageCanvas();
    connectFileServiceToImageCanvas();
    connectImageCanvasToFileService();
    connectFileServiceToMainWindowUI();
}

void ApplicationWiring::connectMainWindowToFileService() {
    // User actions -> FileService
    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigOpenFolderRequested,
        &fileService_, [&]() { fileService_.openFolderDialog(DataSet::Auto); });

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigFileActivated,
        &fileService_, &FileService::openIndex);

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigDroppedPaths,
        &fileService_, &FileService::openPaths);

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigNextRequested,
        &fileService_, [&]() { fileService_.next(); });

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigPrevRequested,
        &fileService_, &FileService::prev);

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigDeleteRequested,
        &fileService_, &FileService::deleteCurrent);

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigGetStasRequested,
        &fileService_, &FileService::getStas);
}

void ApplicationWiring::connectFileServiceToMainWindow() {
    // FileService -> MainWindow
    QObject::connect(
        &fileService_, &FileService::modelReady,
        &mainWindow_, &::ui::MainWindow::setFileModel);

    QObject::connect(
        &fileService_, &FileService::rootChanged,
        &mainWindow_, &::ui::MainWindow::setRoot);

    QObject::connect(
        &fileService_, &FileService::currentIndexChanged,
        &mainWindow_, &::ui::MainWindow::setCurrentIndex);

    QObject::connect(
        &fileService_, &FileService::imageReady,
        &mainWindow_, &::ui::MainWindow::showImage);

    QObject::connect(
        &fileService_, &FileService::status,
        &mainWindow_, &::ui::MainWindow::setStatus);

    QObject::connect(
        &fileService_, &FileService::busy,
        &mainWindow_, &::ui::MainWindow::setBusy);

    QObject::connect(
        &fileService_, &FileService::StasGetted,
        &mainWindow_, &::ui::MainWindow::sigStasUpdateRequested);

    QObject::connect(
        &fileService_, &FileService::saveRequested,
        &mainWindow_, &::ui::MainWindow::sigSaveRequested);
}

void ApplicationWiring::connectMainWindowInternal() {
    // MainWindow internal connections
    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigSettingsRequested,
        &mainWindow_, &::ui::MainWindow::showSettingDialog);

    QObject::connect(
        &mainWindow_, &::ui::MainWindow::sigHistEqRequested,
        mainWindow_.ui()->label, &::ImageCanvas::histEqualize);
}

void ApplicationWiring::connectImageCanvasToDetector() {
    // ImageCanvas -> Detector
    QObject::connect(
        mainWindow_.ui()->label, &::ImageCanvas::detectRequested,
        &detector_, &SmartDetector::detect);
}

void ApplicationWiring::connectDetectorToImageCanvas() {
    // Detector -> ImageCanvas
    QObject::connect(
        &detector_, &SmartDetector::detected,
        mainWindow_.ui()->label, &::ImageCanvas::setDetections);
}

void ApplicationWiring::connectFileServiceToImageCanvas() {
    // FileService -> ImageCanvas
    QObject::connect(
        &fileService_, &FileService::labelsLoaded,
        mainWindow_.ui()->label, &::ImageCanvas::setDetections);

    QObject::connect(
        &fileService_, &FileService::labelTextChanged,
        &mainWindow_, &::ui::MainWindow::setLabelContent);
}

void ApplicationWiring::connectImageCanvasToFileService() {
    // ImageCanvas -> FileService
    QObject::connect(
        mainWindow_.ui()->label, &::ImageCanvas::annotationsPublished,
        &fileService_, &FileService::saveData);
}

void ApplicationWiring::connectFileServiceToMainWindowUI() {
    // FileService -> MainWindow UI elements
    // Currently handled by connectFileServiceToMainWindow
}

} // namespace labelmaster::core
