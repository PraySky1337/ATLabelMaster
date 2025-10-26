#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include "ui_mainwindow.h"

namespace ui {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // === 对外接口（逻辑层可调用） ===
    void showImage(const QPixmap& pix);
    void appendLog(const QString& text);
    void setStatus(const QString& text);
    void loadDirectory(const QString& path);

signals:
    // === 逻辑层可连接的信号 ===
    void openFolderClicked();
    void smartAnnotateClicked();
    void previousClicked();
    void nextClicked();
    void deleteClicked();
    void saveClicked();

private:
    Ui::MainWindow* ui;
    QFileSystemModel* fileModel_;

    void setupConnections();
};

} // namespace labelmaster
