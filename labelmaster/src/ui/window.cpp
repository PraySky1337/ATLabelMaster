#include "ui/window.hpp"
#include "dataset_manager.hpp"
#include "label_storage.hpp"
#include "ui/image_canvas.hpp"

// 只在 cpp 里包含 uic 头
#include "ui_mainwindow.h"

#include "app_settings.hpp"
#include "settings_dialog.hpp"

#include <QAction>
#include <QDateTime>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QMessageBox>
#include <QResizeEvent>
#include <QShortcut>

namespace ui {

static bool isImageFile(const QString& path) {
    const QString p = path.toLower();
    return p.endsWith(".png") || p.endsWith(".jpg") || p.endsWith(".jpeg") || p.endsWith(".bmp");
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fileModel_(new QFileSystemModel(this)) {

    ui->setupUi(this);
    setWindowTitle("ATLabelMaster - 数据标注工具");

    // 仅显示图片
    fileModel_->setNameFilters({"*.png", "*.jpg", "*.jpeg", "*.bmp"});
    fileModel_->setNameFilterDisables(false);
    fileModel_->setRootPath(QDir::homePath());

    ui->file_tree_view->setModel(fileModel_);
    ui->file_tree_view->setRootIndex(fileModel_->index(QDir::homePath()));
    ui->file_tree_view->setColumnWidth(0, 250);
    ui->file_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->log_text->setReadOnly(true);

    // 连接 UI 控件
    setupConnections();
    setupActions();

    // 保存按钮（无 ROI/无类别选择版本：整图作为 ROI，类别固定为 "armor"）
    connect(ui->save_button, &QPushButton::clicked, this, [&] {
        if (imageList_.isEmpty() || currentIndex_ < 0)
            return;

        auto& mgr         = DatasetManager::instance();
        QString targetDir = DatasetManager::instance().saveDir();
        if (targetDir.isEmpty())
            targetDir = AppSettings::instance().saveDir();
        if (targetDir.isEmpty()) {
            targetDir = QFileDialog::getExistingDirectory(this, "选择保存数据集目录");
            if (targetDir.isEmpty())
                return;
            AppSettings::instance().setSaveDir(targetDir);
        }
        DatasetManager::instance().setSaveDir(targetDir);

        const QString imgPath = imageList_[currentIndex_];
        QImage img(imgPath);
        if (img.isNull()) {
            QMessageBox::warning(this, "错误", "无法读取图片，保存失败。");
            return;
        }

        // 整图作为 ROI
        QRect fullRoi(0, 0, img.width(), img.height());

        // 你的接口：exportPatch 返回 void，直接调用即可
        exportPatch(imgPath, fullRoi, mgr.saveDir());

        // 你的接口：saveYoloLabel 需要 imagePath + roi + cls + saveDir
        const QString cls = "armor"; // 先固定类别
        saveYoloLabel(imgPath, fullRoi, cls, mgr.saveDir());

        // 保存进度 & 日志
        mgr.saveProgress(currentIndex_);
        appendLog(QString("已保存整图：%1").arg(QFileInfo(imgPath).fileName()));
        setStatus("保存成功");
    });

    connect(ui->pushButton, &QPushButton::clicked, this, [this] {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            // 需要时应用部分设置，例如保存目录：
            // DatasetManager 优先用 settings 的保存路径
            // （也可以在真正保存时读取 AppSettings）
        }
    });

    // 初次载入时尝试恢复进度
    auto& mgr = DatasetManager::instance();
    if (!mgr.imageDir().isEmpty()) {
        QDir test(mgr.imageDir());
        if (test.exists()) {
            loadDirectory(mgr.imageDir());
            int last = mgr.loadProgress();
            if (last >= 0 && last < imageList_.size()) {
                currentIndex_ = last;
                showImageAt(currentIndex_);
                syncTreeToCurrent();
                appendLog(QString("恢复进度：第 %1 张").arg(last + 1));
            }
        }
    }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setPixelSize(int width, int height) {
    if (ui->label) {
        ui->label->setMinimumSize(QSize(width, height));
    }
}

void MainWindow::setupConnections() {
    connect(ui->open_folder_button, &QPushButton::clicked, this, &MainWindow::onOpenFolderClicked);
    connect(ui->previous_button, &QPushButton::clicked, this, &MainWindow::onPreviousClicked);
    connect(ui->next_pic, &QPushButton::clicked, this, &MainWindow::onNextClicked);

    // 树点击/回车/双击 -> 打开该图片（若是图片文件）
    connect(ui->file_tree_view, &QTreeView::clicked, this, &MainWindow::openFromTreeIndex);
    connect(ui->file_tree_view, &QTreeView::activated, this, &MainWindow::openFromTreeIndex);
}

void MainWindow::setupActions() {
    auto prevAct = new QAction(this);
    auto nextAct = new QAction(this);

    prevAct->setShortcuts(
        {QKeySequence(Qt::Key_Left), QKeySequence(Qt::Key_A), QKeySequence(Qt::Key_J),
         QKeySequence(Qt::Key_PageUp)});
    nextAct->setShortcuts(
        {QKeySequence(Qt::Key_Right), QKeySequence(Qt::Key_D), QKeySequence(Qt::Key_K),
         QKeySequence(Qt::Key_PageDown)});

    addAction(prevAct);
    addAction(nextAct);
    connect(prevAct, &QAction::triggered, this, &MainWindow::onPreviousClicked);
    connect(nextAct, &QAction::triggered, this, &MainWindow::onNextClicked);

    // 空格下一张
    auto spaceNext = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(spaceNext, &QShortcut::activated, this, &MainWindow::onNextClicked);
}

void MainWindow::onOpenFolderClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择图片文件夹");
    if (dir.isEmpty())
        return;
    loadDirectory(dir);
}

void MainWindow::loadDirectory(const QString& path) {
    currentDir_ = path;
    imageList_.clear();
    pathToIndex_.clear();

    // 收集目录下所有图片
    QDirIterator it(path, {"*.png", "*.jpg", "*.jpeg", "*.bmp"}, QDir::Files);
    while (it.hasNext()) {
        const QString p = it.next();
        imageList_.append(p);
        pathToIndex_.insert(p, imageList_.size() - 1);
    }

    if (imageList_.isEmpty()) {
        QMessageBox::warning(this, "提示", "该目录下没有图片文件。");
        return;
    }

    // 更新树根
    fileModel_->setRootPath(path);
    ui->file_tree_view->setRootIndex(fileModel_->index(path));

    currentIndex_ = 0;
    showImageAt(currentIndex_);
    syncTreeToCurrent();

    appendLog(QString("加载目录：%1，共 %2 张图片").arg(path).arg(imageList_.size()));
    setStatus(QString("已加载 %1 张图片").arg(imageList_.size()));

    // 记录当前图片目录（用于 progress 绑定）
    DatasetManager::instance().setImageDir(path);
    AppSettings::instance().setLastImageDir(path);
}

void MainWindow::onPreviousClicked() {
    if (imageList_.isEmpty())
        return;
    currentIndex_ = (currentIndex_ > 0) ? (currentIndex_ - 1) : (imageList_.size() - 1);
    showImageAt(currentIndex_);
    syncTreeToCurrent();
}

void MainWindow::onNextClicked() {
    if (imageList_.isEmpty())
        return;
    currentIndex_ = (currentIndex_ + 1) % imageList_.size();
    showImageAt(currentIndex_);
    syncTreeToCurrent();
}

void MainWindow::showImageAt(int index) {
    if (index < 0 || index >= imageList_.size())
        return;

    const QString path = imageList_[index];
    if (auto canvas = qobject_cast<ImageCanvas*>(ui->label)) {
        if (!canvas->loadImage(path)) {
            appendLog(QString("无法加载图片：%1").arg(path));
            return;
        }
    } else {
        // 兼容：如果还没 promote 成 ImageCanvas，就退回 QLabel 显示
        QPixmap pix(path);
        if (pix.isNull()) {
            appendLog(QString("无法加载图片：%1").arg(path));
            return;
        }
        ui->label->setPixmap(
            pix.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    appendLog(QString("显示图片：%1").arg(path));
    setStatus(QString("第 %1/%2 张").arg(index + 1).arg(imageList_.size()));
}
void MainWindow::syncTreeToCurrent() {
    if (currentIndex_ < 0 || currentIndex_ >= imageList_.size())
        return;
    const QString path   = imageList_[currentIndex_];
    const QModelIndex mi = fileModel_->index(path);
    if (!mi.isValid())
        return;

    auto* sel = ui->file_tree_view->selectionModel();
    sel->setCurrentIndex(mi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ui->file_tree_view->scrollTo(mi, QAbstractItemView::PositionAtCenter);
}

void MainWindow::openFromTreeIndex(const QModelIndex& idx) {
    if (!idx.isValid())
        return;
    QFileInfo fi = fileModel_->fileInfo(idx);
    if (fi.isDir())
        return;
    const QString path = fi.absoluteFilePath();
    if (!isImageFile(path))
        return;

    if (pathToIndex_.contains(path)) {
        currentIndex_ = pathToIndex_.value(path);
        showImageAt(currentIndex_);
    } else {
        // 跨目录：切换并定位
        loadDirectory(fi.absolutePath());
        if (pathToIndex_.contains(path)) {
            currentIndex_ = pathToIndex_.value(path);
            showImageAt(currentIndex_);
            syncTreeToCurrent();
        }
    }
}

void MainWindow::appendLog(const QString& text) {
    QString line =
        QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(text);
    ui->log_text->append(line);
}

void MainWindow::setStatus(const QString& text) { statusBar()->showMessage(text, 3000); }

void MainWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
    // 让图片随窗口调整重新等比显示
    if (currentIndex_ >= 0 && currentIndex_ < imageList_.size() && ui->label
        && !ui->label->pixmap(Qt::ReturnByValue).isNull()) {
        QPixmap pix(imageList_[currentIndex_]);
        if (!pix.isNull()) {
            ui->label->setPixmap(
                pix.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

} // namespace ui
