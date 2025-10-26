#include "window.hpp"
#include <QFileDialog>
#include <QDateTime>

namespace ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fileModel_(new QFileSystemModel(this))
{
    ui->setupUi(this);
    setWindowTitle("ATLabelMaster - 数据标注工具");

    // 初始化文件树
    fileModel_->setRootPath(QDir::homePath());
    ui->file_tree_view->setModel(fileModel_);
    ui->file_tree_view->setRootIndex(fileModel_->index(QDir::homePath()));
    ui->file_tree_view->setColumnWidth(0, 250);

    // 日志框只读
    ui->log_text->setReadOnly(true);

    setupConnections();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupConnections() {
    connect(ui->open_folder_button, &QPushButton::clicked, this, &MainWindow::openFolderClicked);
    connect(ui->smart_button,       &QPushButton::clicked, this, &MainWindow::smartAnnotateClicked);
    connect(ui->previous_button,    &QPushButton::clicked, this, &MainWindow::previousClicked);
    connect(ui->next_pic,           &QPushButton::clicked, this, &MainWindow::nextClicked);
    connect(ui->delete_button,      &QPushButton::clicked, this, &MainWindow::deleteClicked);
    connect(ui->save_button,        &QPushButton::clicked, this, &MainWindow::saveClicked);

    // 文件树双击打开
    connect(ui->file_tree_view, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        QString path = fileModel_->filePath(index);
        emit nextClicked();  // 或者自定义信号，如 emit fileSelected(path);
        appendLog(QString("打开文件：%1").arg(path));
    });
}

void MainWindow::showImage(const QPixmap& pix) {
    ui->label->setPixmap(pix.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::appendLog(const QString& text) {
    QString line = QString("[%1] %2")
                       .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                       .arg(text);
    ui->log_text->append(line);
}

void MainWindow::setStatus(const QString& text) {
    statusBar()->showMessage(text, 3000);
}

void MainWindow::loadDirectory(const QString& path) {
    fileModel_->setRootPath(path);
    ui->file_tree_view->setRootIndex(fileModel_->index(path));
    appendLog(QString("加载目录：%1").arg(path));
}

} // namespace labelmaster
