// ===============================
// File: service/file.hpp
// ===============================
#pragma once
#include "../dataset/dataset.h"
#include "types.hpp"    // Armor 定义
#include <QModelIndex>
#include <QObject>
#include <QPersistentModelIndex>
#include <QSize>
#include <QStringList>
#include <QVector>
#include <qaction.h>
#include <qobject.h>

class QAbstractItemModel;
class QFileSystemModel;
class QSortFilterProxyModel;
class QImage;

class FileService : public QObject {
    Q_OBJECT
public:
    explicit FileService(QObject* parent = nullptr);
    ~FileService() override;

    void exposeModel(); // 把 proxy 模型抛给 UI

public slots:
    // === 打开 ===
    void openFolderDialog(const DataSet& type = DataSet::LabelMaster2); // 弹框选目录
    void importFrom(const QAction* action);                             // 导入其他数据集
    void openPaths(const QStringList&);                                 // 拖拽/命令行路径
    void openIndex(const QModelIndex&);                                 // 由文件树激活

    // === 浏览 ===
    void next(bool allowAutoSave = true);
    void prev();

    // === 修改 ===
    void deleteCurrent(); // 直接删除当前文件（简单实现）

    // === 保存标注 ===
    void saveData(const QVector<Armor>& armors, const QImage& image, bool needSaveImg);

    // === 获取统计信息 ==
    void getStas(int colorId, int classId, int sizeId);

signals:
    // === 给 UI 的输出 ===
    void modelReady(QAbstractItemModel* proxyModel);
    void rootChanged(const QModelIndex& proxyRoot);
    void currentIndexChanged(const QModelIndex& proxyIndex);
    void imageReady(const QImage& img);
    void status(const QString& msg, int ms = 1500);
    void busy(bool on);

    // === 打开图片时加载到的标注 ===
    void labelsLoaded(const QVector<Armor>& armors);
    // ===统计信息获取==
    void StasGetted(const int& targetCount, const int& fileCount);
    // ===自动保存===
    void saveRequested();

private:
    // 目录加载完成后再尝试选第一张
    void selectFirst(const QString& path);
    bool openDir(const QString& dir);
    bool openFileAt(const QModelIndex& proxyIndex);
    bool tryImportDataSetAfterLoaded();
    void tryOpenFirstAfterLoaded(const QString& dir);
    QModelIndex findFirstImageUnder(const QModelIndex& proxyRoot) const;
    QModelIndex mapFromProxyToSource(const QModelIndex&) const;
    QModelIndex mapFromSourceToProxy(const QModelIndex&) const;
    bool isImageFile(const QString& path) const;

    // 记忆 & 恢复
    void saveLastVisited(const QString& imagePath);
    void tryRestoreLastVisited(); // 异步调用
    bool setProxyRoot(const QString& dir);

    // 标注 I/O（归一化支持）
    static QString labelFileForImage(const QString& imagePath);
    static bool writeLabelFile(
        const QString& labelPath, const QVector<Armor>& armors,
        const QSize& imgSize);                                         // 保存为归一化
    static QVector<Armor>
        readLabelFile(const QString& labelPath, const QSize& imgSize); // 自动反归一化

private:
    QString pendingDir_;
    QString pendingTargetPath_;
    QFileSystemModel* fsModel_    = nullptr;                           // 源模型
    QSortFilterProxyModel* proxy_ = nullptr;                           // 只显示图片与目录
    QPersistentModelIndex proxyRoot_;
    QPersistentModelIndex proxyCurrent_;
    QString currentImagePath_;                                         // 当前图片绝对路径
    QSize currentImageSize_;                                           // 当前图片尺寸（归一化需要）
    DataSet currentDataSet = DataSet::LabelMaster2;
};
