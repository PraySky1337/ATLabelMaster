#pragma once
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileSystemModel>
#include <QHash>
#include <QMainWindow>
#include <QPixmap>
#include <QStringList>

namespace ui {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    // 这个方法看起来像外部会调用的工具方法，放到 public 更合适
    void setPixelSize(int width, int height);

private slots:
    // === 槽函数 ===
    void onOpenFolderClicked();
    void onPreviousClicked();
    void onNextClicked();

private:
    Ui::MainWindow* ui;
    QFileSystemModel* fileModel_;

    // 当前目录和图片列表
    QString currentDir_;
    QStringList imageList_;
    int currentIndex_ = -1;

    // 新增：路径 -> index 的快速映射，以及当前原图（用于比例缩放）
    QHash<QString, int> pathToIndex_;
    QPixmap currentPix_;

    // === 内部方法 ===
    void setupConnections();
    void setupActions(); // 新增：键盘快捷键/动作
    void loadDirectory(const QString& path);
    void showImageAt(int index);
    void appendLog(const QString& text);
    void setStatus(const QString& text);

    // 新增：树与当前图片索引的互相同步
    void syncTreeToCurrent();
    void openFromTreeIndex(const QModelIndex& idx);

protected:
    // 新增：窗口/label 尺寸变化时保持等比例缩放
    void resizeEvent(QResizeEvent* e) override;
};

} // namespace ui
