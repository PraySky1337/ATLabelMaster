#include "ui/window.hpp"
#include <QApplication>
#include <QSettings>
#include <QCoreApplication>
#include <QMainWindow>

#define APP_NAME "ATLabelMaster"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(APP_NAME);
    QCoreApplication::setApplicationName(APP_NAME);

    ui::MainWindow mainwindow;
    mainwindow.show();
    return app.exec();
}