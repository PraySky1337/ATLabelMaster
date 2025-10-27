#pragma once
#include <QObject>
#include <QModelIndex>
#include <QStringList>
#include <memory>

class QAbstractItemModel;
class QFileSystemModel;
class QSortFilterProxyModel;
class QImage;

// 仅负责“文件/图片浏览”这条线：目录->模型->当前图片
class FileService : public QObject {
    Q_OBJECT
public:
    explicit FileService(QObject* parent = nullptr);
    ~FileService() override;
    void exposeModel();  

public slots:
    // === 打开 ===
    void openFolderDialog();              // 弹框选目录
    void openPaths(const QStringList&);   // 拖拽/命令行路径
    void openIndex(const QModelIndex&);   // 由文件树激活

    // === 浏览 ===
    void next();
    void prev();

    // === 修改 ===
    void deleteCurrent();                 // 直接删除当前文件（简单实现）

signals:
    // === 给 UI 的输出 ===
    void modelReady(QAbstractItemModel* proxyModel); // 交给 QTreeView setModel
    void rootChanged(const QModelIndex& proxyRoot); 
    void currentIndexChanged(const QModelIndex& proxyIndex);
    void imageReady(const QImage& img);
    void status(const QString& msg, int ms = 1500);
    void busy(bool on);

private:
    bool openDir(const QString& dir);
    bool openFileAt(const QModelIndex& proxyIndex);
    void tryOpenFirstAfterLoaded(const QString& dir);
    QModelIndex findFirstImageUnder(const QModelIndex& proxyRoot) const;
    QModelIndex mapFromProxyToSource(const QModelIndex&) const;
    QModelIndex mapFromSourceToProxy(const QModelIndex&) const;
    bool isImageFile(const QString& path) const;

private:
    QString pendingDir_;
    QString pendingTargetPath_;
    QFileSystemModel*        fsModel_     = nullptr; // 源模型
    QSortFilterProxyModel*   proxy_       = nullptr; // 只显示图片与目录
    QModelIndex              proxyRoot_;
    QModelIndex              proxyCurrent_;
};
