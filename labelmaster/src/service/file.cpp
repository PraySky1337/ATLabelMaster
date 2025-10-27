#include "service/file.hpp"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QImage>
#include <QImageReader>
#include <QQueue>
#include <QSortFilterProxyModel>

#include "controller/dataset.hpp"  // 用于恢复/保存浏览进度（可选）
#include "controller/settings.hpp" // 用于恢复/记录最后目录（可选）
#include "logger/core.hpp"         // 复用你的全局 Logger

namespace {
static const QStringList kImgExt = {"*.png", "*.jpg", "*.jpeg", "*.bmp",
                                    "*.gif", "*.tif", "*.tiff", "*.webp"};

class ImageFilterProxy : public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    bool filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const override {
        const auto idx = sourceModel()->index(srcRow, 0, srcParent);
        if (!idx.isValid())
            return false;

        const auto* fsm = qobject_cast<const QFileSystemModel*>(sourceModel());
        if (!fsm)
            return true;

        if (fsm->isDir(idx))
            return true;           // 目录保留
        const QString name = fsm->fileName(idx).toLower();
        for (const auto& pat : kImgExt) {
            // endsWith(pat 去掉 * )
            if (name.endsWith(pat.mid(1)))
                return true;
        }
        return false;
    }
};
} // namespace

void FileService::exposeModel() { emit modelReady(proxy_); }

bool FileService::openDir(const QString& dir) {
    emit busy(true);

    pendingDir_ = dir;
    pendingTargetPath_.clear();

    const QModelIndex srcRoot = fsModel_->setRootPath(dir); // 异步开始
    if (!srcRoot.isValid()) {
        LOGW(QString("无效目录：%1").arg(dir));
        emit busy(false);
        return false;
    }

    // 立刻计算一次 proxyRoot_（可能还没孩子）
    proxyRoot_ = mapFromSourceToProxy(srcRoot);
    if (proxyRoot_.isValid() && proxyRoot_.model() == proxy_) {
        emit rootChanged(proxyRoot_);   // 先让视图切 root
    }

    emit status(tr("打开目录：%1").arg(dir));
    LOGI(QString("打开目录：%1").arg(dir));

    controller::AppSettings::instance().setLastImageDir(dir);
    controller::DatasetManager::instance().setImageDir(dir);

    // 目录小的话此时可能已经可用，尝试一次；否则等 directoryLoaded 触发再次尝试
    tryOpenFirstAfterLoaded(dir);
    return true; // busy 在 tryOpenFirstAfterLoaded 成功/失败时关闭
}

void FileService::tryOpenFirstAfterLoaded(const QString& dir) {
    if (!fsModel_ || !proxy_) return;

    // 1) 用当前目录重新计算 root，避免使用过期/异模 index
    QModelIndex srcRoot = fsModel_->index(dir);
    if (!srcRoot.isValid()) return;

    QModelIndex pxRoot = mapFromSourceToProxy(srcRoot);
    if (!pxRoot.isValid()) return;

    // 2) 保证 index 来自 proxy_（关键）
    if (pxRoot.model() != proxy_) return;

    proxyRoot_ = pxRoot;

    // 3) 如果还没加载出子项，等下一次 directoryLoaded
    const int rows = proxy_->rowCount(proxyRoot_);
    if (rows == 0) return;

    // 4) 如果拖的是“具体文件”，优先直接定位
    if (!pendingTargetPath_.isEmpty()) {
        const QModelIndex srcIdx = fsModel_->index(pendingTargetPath_);
        if (srcIdx.isValid() && !fsModel_->isDir(srcIdx)) {
            const QModelIndex px = mapFromSourceToProxy(srcIdx);
            if (px.isValid() && px.model() == proxy_) {
                proxyCurrent_ = px;
                emit currentIndexChanged(proxyCurrent_);
                openFileAt(proxyCurrent_);
                emit busy(false);
                pendingDir_.clear();
                pendingTargetPath_.clear();
                return;
            }
        }
        // 定位失败则退化为第一张
        pendingTargetPath_.clear();
    }

    // 5) 正常找第一张图片
    const QModelIndex target = findFirstImageUnder(proxyRoot_);
    if (target.isValid()) {
        proxyCurrent_ = target;
        emit currentIndexChanged(proxyCurrent_);
        openFileAt(proxyCurrent_);
        emit busy(false);
        pendingDir_.clear();
    } else {
        LOGW(QString("目录下未找到图片：%1").arg(dir));
        emit status(tr("目录下未找到图片"), 1200);
        emit busy(false);
        pendingDir_.clear();
    }
}



FileService::FileService(QObject* parent)
    : QObject(parent)
    , fsModel_(new QFileSystemModel(this))
    , proxy_(new ImageFilterProxy(this)) {

    fsModel_->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    fsModel_->setNameFilterDisables(false);
    fsModel_->setNameFilters(kImgExt);

    proxy_->setSourceModel(fsModel_);
    proxy_->setRecursiveFilteringEnabled(true);
    proxy_->setDynamicSortFilter(true);

    // 目录加载完成后再尝试选第一张
    connect(
        fsModel_, &QFileSystemModel::directoryLoaded, this,
        [this](const QString& path) {
            if (pendingDir_.isEmpty())
                return;
            // 允许子目录逐步加载；这里仅当包含关系成立时尝试
            if (path == pendingDir_ || path.startsWith(pendingDir_ + '/')) {
                tryOpenFirstAfterLoaded(pendingDir_);
            }
        },
        Qt::UniqueConnection);
}
FileService::~FileService() = default;

// ------ 打开入口 ------

void FileService::openFolderDialog() {
    const QString dir = QFileDialog::getExistingDirectory(nullptr, tr("选择图片文件夹"));
    if (dir.isEmpty())
        return;
    openDir(dir);
}

// BFS 找第一张图片（跨多层）
QModelIndex FileService::findFirstImageUnder(const QModelIndex& root) const {
    if (!root.isValid())
        return {};
    QQueue<QModelIndex> q;
    q.enqueue(root);

    while (!q.isEmpty()) {
        QModelIndex p  = q.dequeue();
        const int rows = proxy_->rowCount(p);
        for (int r = 0; r < rows; ++r) {
            QModelIndex idx = proxy_->index(r, 0, p);
            QModelIndex s   = mapFromProxyToSource(idx);
            if (!s.isValid())
                continue;

            if (fsModel_->isDir(s)) {
                q.enqueue(idx);
            } else {
                const QString path = fsModel_->filePath(s);
                if (isImageFile(path))
                    return idx;
            }
        }
    }
    return {};
}

bool FileService::openFileAt(const QModelIndex& proxyIndex) {
    const QModelIndex s = mapFromProxyToSource(proxyIndex);
    if (!s.isValid() || fsModel_->isDir(s))
        return false;

    const QString path = fsModel_->filePath(s);
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) {
        LOGE(QString("加载失败：%1 (%2)").arg(path, reader.errorString()));
        emit status(tr("加载失败：%1").arg(reader.errorString()), 1500);
        return false;
    }

    emit imageReady(img);
    emit status(tr("已打开：%1").arg(QFileInfo(path).fileName()), 800);

    // 保存浏览进度（可选：这里只保存“有打开过”这个事实）
    controller::DatasetManager::instance().saveProgress(/*index=*/0);
    return true;
}

void FileService::openIndex(const QModelIndex& proxyIndex) {
    if (!proxyIndex.isValid())
        return;
    proxyCurrent_ = proxyIndex;
    emit currentIndexChanged(proxyCurrent_);
    openFileAt(proxyCurrent_);
}

// ------ 浏览 ------

void FileService::next() {
    if (!proxyCurrent_.isValid())
        return;

    // 在同级中找下一张图片；到尾部则提示
    const QModelIndex parent =
        proxyCurrent_.parent().isValid() ? proxyCurrent_.parent() : proxyRoot_;
    int r          = proxyCurrent_.row() + 1;
    const int rows = proxy_->rowCount(parent);
    for (; r < rows; ++r) {
        const QModelIndex idx = proxy_->index(r, 0, parent);
        const QModelIndex s   = mapFromProxyToSource(idx);
        if (s.isValid() && !fsModel_->isDir(s) && isImageFile(fsModel_->filePath(s))) {
            proxyCurrent_ = idx;
            emit currentIndexChanged(proxyCurrent_);
            openFileAt(proxyCurrent_);
            return;
        }
    }
    emit status(tr("已经是最后一张"), 900);
}

void FileService::prev() {
    if (!proxyCurrent_.isValid())
        return;

    const QModelIndex parent =
        proxyCurrent_.parent().isValid() ? proxyCurrent_.parent() : proxyRoot_;
    int r = proxyCurrent_.row() - 1;
    for (; r >= 0; --r) {
        const QModelIndex idx = proxy_->index(r, 0, parent);
        const QModelIndex s   = mapFromProxyToSource(idx);
        if (s.isValid() && !fsModel_->isDir(s) && isImageFile(fsModel_->filePath(s))) {
            proxyCurrent_ = idx;
            emit currentIndexChanged(proxyCurrent_);
            openFileAt(proxyCurrent_);
            return;
        }
    }
    emit status(tr("已经是第一张"), 900);
}

// ------ 删除 ------

void FileService::deleteCurrent() {
    if (!proxyCurrent_.isValid())
        return;
    const QModelIndex s = mapFromProxyToSource(proxyCurrent_);
    if (!s.isValid() || fsModel_->isDir(s))
        return;

    const QString path = fsModel_->filePath(s);
    if (QFile::remove(path)) {
        LOGW(QString("已删除：%1").arg(path));
        // 模型会刷新；尝试打开下一张
        next();
    } else {
        LOGE(QString("删除失败：%1").arg(path));
        emit status(tr("删除失败"), 1200);
    }
}

// ------ 工具方法 ------

QModelIndex FileService::mapFromProxyToSource(const QModelIndex& p) const {
    return proxy_->mapToSource(p);
}
QModelIndex FileService::mapFromSourceToProxy(const QModelIndex& s) const {
    return proxy_->mapFromSource(s);
}
bool FileService::isImageFile(const QString& path) const {
    const QString low = path.toLower();
    for (const auto& ext : kImgExt)
        if (low.endsWith(ext.mid(1)))
            return true;
    return false;
}

void FileService::openPaths(const QStringList& paths) {
    if (paths.isEmpty()) return;

    QString dir;
    pendingTargetPath_.clear();

    for (QString p : paths) {
        // 兼容 "file://..." 形式
        if (p.startsWith("file://")) {
            QUrl u(p);
            if (u.isLocalFile()) p = u.toLocalFile();
        }
        QFileInfo fi(p);
        if (!fi.exists()) continue;

        if (fi.isDir()) {
            // 有目录就以目录为准；目标文件清空
            dir = fi.absoluteFilePath();
            pendingTargetPath_.clear();
            break;
        } else if (fi.isFile()) {
            if (dir.isEmpty()) dir = fi.absolutePath();
            if (pendingTargetPath_.isEmpty()) pendingTargetPath_ = fi.absoluteFilePath();
        }
    }

    if (!dir.isEmpty()) {
        openDir(dir);  // 异步：目录加载完成后在 tryOpenFirstAfterLoaded() 里处理 pendingTargetPath_
    }
}