/**
 * @file application_wiring.hpp
 * @brief Application wiring - encapsulates all signal/slot connections
 *
 * Centralizes the wiring between major application components
 * (MainWindow, FileService, SmartDetector) to improve maintainability.
 */

#ifndef LABELMASTER_APPLICATION_WIRING_HPP
#define LABELMASTER_APPLICATION_WIRING_HPP

#include <QObject>

// Forward declarations - ui namespace is at global scope
namespace ui {
class MainWindow;
}

class FileService;
class SmartDetector;

namespace labelmaster::core {

/**
 * @brief Application wiring class
 *
 * Encapsulates all signal/slot connections between major components.
 * This makes the codebase more maintainable by centralizing connection logic.
 */
class ApplicationWiring : public QObject {
    Q_OBJECT

public:
    ApplicationWiring(
        ::ui::MainWindow& mainWindow,
        FileService& fileService,
        SmartDetector& detector,
        QObject* parent = nullptr);

    /**
     * @brief Establish all signal/slot connections
     */
    void wire();

private:
    void connectMainWindowToFileService();
    void connectFileServiceToMainWindow();
    void connectMainWindowInternal();
    void connectImageCanvasToDetector();
    void connectDetectorToImageCanvas();
    void connectFileServiceToImageCanvas();
    void connectImageCanvasToFileService();
    void connectFileServiceToMainWindowUI();

private:
    ::ui::MainWindow& mainWindow_;
    FileService& fileService_;
    SmartDetector& detector_;
};

} // namespace labelmaster::core

#endif // LABELMASTER_APPLICATION_WIRING_HPP
