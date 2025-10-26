#include <QApplication>
#include <QMainWindow>
#include "ui/window.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ui::MainWindow mainwindow;
    mainwindow.show();
    return app.exec();
}