// ===============================
// File: service/file.cpp
// ===============================
#include "service/file.hpp"
#include "../util/id_convert.hpp"
#include "detector/ai/detector.hpp"
#include "types.hpp"
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QImage>
#include <QImageReader>
#include <QQueue>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <cstdio>
#include <qabstractitemmodel.h>
#include <qbuffer.h>
#include <qcontiguouscache.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qhashfunctions.h>
#include <qimage.h>
#include <qiodevicebase.h>
#include <qlist.h>
#include <qmath.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpointer.h>
#include <qsettings.h>
#include <qsharedpointer.h>
#include <qsortfilterproxymodel.h>
#include <qstringalgorithms.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <strings.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# include <QStringConverter> // Qt6: QTextStream::setEncoding
#endif
#include <QTextStream>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <cmath>

#include "../ui/stas_dialog.h"
#include "../util/string.hpp"
#include "controller/settings.hpp"
#include "logger/core.hpp"

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
            return true;     // 保留目录
        const QString name = fsm->fileName(idx).toLower();
        for (const auto& pat : kImgExt) {
            if (name.endsWith(pat.mid(1)))
                return true; // endsWith(".png")
        }
        return false;
    }
};
} // namespace

// ---------- 构造 / 析构 ----------
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
    // Don't use DirectoryLoaded , need sort
    connect(
        fsModel_, &QFileSystemModel::directoryLoaded, this, &FileService::selectFirst,
        Qt::UniqueConnection);

    // proxy 重置时清空索引与当前路径，避免悬空
    connect(proxy_, &QAbstractItemModel::modelAboutToBeReset, this, [this] {
        proxyCurrent_ = QPersistentModelIndex();
        proxyRoot_    = QPersistentModelIndex();
        currentImagePath_.clear();
        currentImageSize_ = {};
    });

    // 异步尝试恢复上次图片（避免构造期阻塞）
    QTimer::singleShot(0, this, &FileService::tryRestoreLastVisited);
}
FileService::~FileService() = default;

// ---------- 模型暴露 ----------
void FileService::exposeModel() { emit modelReady(proxy_); }

void FileService::importFrom(const QAction* action) {
    DataSet dataset;
    if (action->objectName() == "actionLMV1") {
        dataset = DataSet::LabelMaster;
    } else if (action->objectName() == "actionLMV2") {
        dataset = DataSet::LabelMaster2;
    } else if (action->objectName() == "actionHITSZ") { // 南工骁鹰
        dataset = DataSet::HITSZ;
    } else if (action->objectName() == "actionUPC") {
        dataset = DataSet::UPC;
    } else if (action->objectName() == "actionNWPU") {
        dataset = DataSet::NWPU;
    }
    openFolderDialog(dataset);
}
// ---------- 打开入口 ----------

void FileService::openFolderDialog(const DataSet& type) {
    const QString dir = QFileDialog::getExistingDirectory(
        nullptr, tr("选择图片文件夹"), QString(),
        QFileDialog::Options(QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly));
    if (dir.isEmpty())
        return;
    currentDataSet = type; // 设置DataSet用于判断是否导入
    openDir(dir);
}
// 目录加载完成后如果需要导入，则进行导入，如果不需要导入再尝试选第一张
void FileService::selectFirst(const QString& path) {
    if (pendingDir_.isEmpty()) {
        emit busy(false);
        return;
    }
    if (!(path == pendingDir_ || path.startsWith(pendingDir_ + '/'))) {
        return;
    };
    if (tryImportDataSetAfterLoaded()) {
        tryOpenFirstAfterLoaded(pendingDir_);
    }
}
// BFS 找第一张图片（跨多层）
QModelIndex FileService::findFirstImageUnder(const QModelIndex& proxyRoot) const {
    if (!proxyRoot.isValid())
        return {};
    QQueue<QModelIndex> q;
    q.enqueue(proxyRoot);

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

    currentImagePath_ = path;       // 记住路径（保存时用）
    currentImageSize_ = img.size(); // 记住尺寸（保存/反归一化）
    saveLastVisited(path);
    const QString lbl = labelFileForImage(path);
    if (QFile::exists(lbl)) {
        QVector<Armor> armors = readLabelFile(lbl, currentImageSize_);
        emit labelsLoaded(armors);
    } else {
        emit labelsLoaded({});
    }
    return true;
}

void FileService::openIndex(const QModelIndex& proxyIndex) {
    if (!proxyIndex.isValid())
        return;
    proxyCurrent_ = proxyIndex;
    emit currentIndexChanged(proxyCurrent_);
    openFileAt(proxyCurrent_);
}

// ---------- 浏览 ----------
void FileService::next(bool allowAutoSave) {
    if (!proxyCurrent_.isValid())
        return;

    QModelIndex parent = proxyCurrent_.parent().isValid() ? proxyCurrent_.parent()
                                                          : static_cast<QModelIndex>(proxyRoot_);
    int r              = proxyCurrent_.row() + 1;
    const int rows     = proxy_->rowCount(parent);
    for (; r < rows; ++r) {
        const QModelIndex idx = proxy_->index(r, 0, parent);
        const QModelIndex s   = mapFromProxyToSource(idx);
        if (s.isValid() && !fsModel_->isDir(s) && isImageFile(fsModel_->filePath(s))) {
            if (controller::AppSettings::instance().autoSave() && allowAutoSave) {
                // 自动保存当前标注
                emit saveRequested();
            }
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

    QModelIndex parent = proxyCurrent_.parent().isValid() ? proxyCurrent_.parent()
                                                          : static_cast<QModelIndex>(proxyRoot_);
    int r              = proxyCurrent_.row() - 1;
    for (; r >= 0; --r) {
        const QModelIndex idx = proxy_->index(r, 0, parent);
        const QModelIndex s   = mapFromProxyToSource(idx);
        if (s.isValid() && !fsModel_->isDir(s) && isImageFile(fsModel_->filePath(s))) {
            if (controller::AppSettings::instance().autoSave()) {
                // 自动保存当前标注
                emit saveRequested();
            }
            proxyCurrent_ = idx;
            emit currentIndexChanged(proxyCurrent_);
            openFileAt(proxyCurrent_);
            return;
        }
    }
    emit status(tr("已经是第一张"), 900);
}

// ---------- 删除 ----------
void FileService::deleteCurrent() {
    if (!proxyCurrent_.isValid())
        return;
    const QModelIndex s = mapFromProxyToSource(proxyCurrent_);
    if (!s.isValid() || fsModel_->isDir(s))
        return;

    const QString path = fsModel_->filePath(s);
    if (QFile::remove(path)) {
        LOGI(QString("已删除：%1").arg(path));
        const QString labelPath = labelFileForImage(path);
        if (QFile::exists(labelPath) && QFile::remove(labelPath)) {
            LOGI(QString("已删除：%1").arg(labelPath));
        }
        next(false);
        if (!proxyCurrent_.isValid()) {
            currentImagePath_.clear();
            currentImageSize_ = {};
        }
    } else {
        LOGE(QString("删除失败：%1").arg(path));
        emit status(tr("删除失败"), 1200);
    }
}

// ---------- 目录打开 ----------
bool FileService::openDir(const QString& dir) {
    emit busy(true);

    QString lastDir = fsModel_->rootPath();
    pendingDir_     = dir; // 不清空 pendingTargetPath_，以便恢复时指定目标文件
    if (lastDir == dir) {
        LOGW(QString("目录已经打开：%1").arg(dir));
        emit busy(false);
        return false;
    }
    const QModelIndex srcRoot = fsModel_->setRootPath(dir);                        // 异步开始
    if (!srcRoot.isValid()) {
        LOGW(QString("无效目录：%1").arg(dir));
        emit busy(false);
        return false;
    }

    proxyRoot_ = mapFromSourceToProxy(srcRoot);
    if (proxyRoot_.isValid() && proxyRoot_.model() == proxy_) {
        emit rootChanged(proxyRoot_);
    }

    emit status(tr("打开目录：%1").arg(dir));
    LOGI(QString("打开目录：%1").arg(dir));
    return true;
}

bool FileService::setProxyRoot(const QString& dir) {
    if (!fsModel_ || !proxy_)
        return false;

    QModelIndex srcRoot = fsModel_->index(dir);
    if (!srcRoot.isValid())
        return false;

    QModelIndex pxRoot = mapFromSourceToProxy(srcRoot);
    if (!pxRoot.isValid())
        return false;

    if (pxRoot.model() != proxy_)
        return false;

    proxyRoot_     = pxRoot;
    const int rows = proxy_->rowCount(pxRoot);
    if (rows == 0)
        return false;
    return true;
}
bool FileService::tryImportDataSetAfterLoaded() {
    if (currentDataSet != DataSet::LabelMaster3) {                                 // 开始导入
        auto fail = [&](const QString& tip, const QString& arg) {
            emit busy(false);
            emit status(tip, 1200);
            LOGE(QString("%1:%2").arg(tip).arg(arg));
        };
        const QModelIndex target = findFirstImageUnder(proxyRoot_);                // 找第一张图片
        if (target.isValid()) {
            for (int i = 0; i < proxy_->rowCount(target.parent()); i++) {
                QString imgPath = fsModel_->filePath(
                    fsModel_->index(i, 0, mapFromProxyToSource(target.parent()))); // 获取图片路径
                const QString labelPath = labelFileForImage(imgPath);              // 计算Label路径
                if (QFile::exists(labelPath)) {
                    QBuffer buffer;
                    buffer.open(QIODevice::ReadWrite);
                    QTextStream convertStream(&buffer);
                    QFile labelFile(labelPath);
                    if (!labelFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        fail("导入失败!无法打开Label", labelPath);
                        return false;
                    }
                    QTextStream ts(&labelFile);
                    switch (currentDataSet) {

                    case DataSet::LabelMaster: {                                   // v1 旧格式
                        // | color | label |
                        // | :---: | :---: |
                        // | 0 |  | G |
                        // | 1 |  | 1 |
                        // | 2 |  | 2 |
                        // | 3 |  | 3 |
                        // | 4 |  | 4 |
                        // | 5  | O(前哨站) |
                        // | 6 | Bs(基地小装甲) |
                        // | 7 | Bb(基地大装甲) |
                        while (!ts.atEnd()) {
                            QString raw = ts.readLine();
                            QStringList t;
                            if (!StringProcess::processLabelString(raw, t)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            int colorId, clsId, sizeId;
                            if (!StringProcess::InitLabelInfo(
                                    t, colorId, clsId, sizeId, currentDataSet)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            t.pop_front();
                            t.pop_front();
                            switch (clsId) {
                            case 6:
                            case 7: clsId = 6; break;
                            }
                            t.push_front(QString().number(clsId));
                            t.push_front(QString().number(sizeId));
                            t.push_front(QString().number(colorId));
                            convertStream << t.join(" ") << "\n";
                        }
                        break;
                    }
                    case DataSet::LabelMaster2:
                        while (!ts.atEnd()) {
                            QString raw = ts.readLine();
                            QStringList t;
                            if (!StringProcess::processLabelString(raw, t)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            int colorId, clsId, sizeId;
                            if (!StringProcess::InitLabelInfo(
                                    t, colorId, clsId, sizeId, currentDataSet)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            t.pop_front();
                            t.pop_front();
                            t.pop_front();
                            switch (clsId) {
                            case 5: continue;
                            case 6:
                            case 7: clsId -= 1; break;
                            }
                            t.push_front(QString().number(clsId));
                            t.push_front(QString().number(sizeId));
                            t.push_front(QString().number(colorId));
                            convertStream << t.join(" ") << "\n";
                        }
                        break;
                    case DataSet::UPC:
                        while (!ts.atEnd()) {
                            QString raw = ts.readLine();
                            QStringList t;
                            if (!StringProcess::processLabelString(raw, t)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            int colorId, clsId, sizeId;
                            if (!StringProcess::InitLabelInfo(
                                    t, colorId, clsId, sizeId, currentDataSet)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            t.pop_front();
                            t.pop_front();
                            switch (clsId) {
                            case 6:
                            case 7: clsId--; break;
                            case 8: clsId -= 2; break;
                            case 9:
                            case 10: clsId -= 6; break;
                            case 5:
                            case 11: continue;
                            }
                            t.push_front(QString().number(clsId));
                            t.push_front(QString().number(sizeId));
                            t.push_front(QString().number(colorId));
                            convertStream << t.join(" ") << "\n";
                        }
                        break;
                    case DataSet::HITSZ: { // 南工骁鹰
                        while (!ts.atEnd()) {
                            QString raw = ts.readLine();
                            QStringList t;
                            if (!StringProcess::processLabelString(raw, t)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            int colorId, clsId, sizeId;
                            if (!StringProcess::InitLabelInfo(
                                    t, colorId, clsId, sizeId, currentDataSet)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            //[目标各点的x、y归一化坐标] <目标类id> <目标颜色id>
                            // 颜色字段:id
                            // 装甲板标注目标ID见下表
                            // 贴纸	ID
                            // G（哨兵）	0
                            // 1（一号）	1
                            // 2（二号）	2
                            // 3（三号）	3
                            // 4（四号）	4
                            // 5（五号）	5
                            // O（前哨站）	6
                            // Bs（基地）	7
                            // Bb（基地大装甲）	8
                            // L3（三号平衡）	9
                            // L4（四号平衡）	10
                            // L5（五号平衡）	11
                            // 颜色ID见下表
                            // 类别	color
                            // Blue	0
                            // Red	1
                            // N（熄灭) 2
                            // Purple	3
                            switch (clsId) {
                            case 6:
                            case 7: clsId--; break;
                            case 8: clsId -= 2; break;
                            case 9:
                            case 10: clsId -= 6; break;
                            case 5:
                            case 11: continue;
                            }
                            t.pop_back();
                            t.pop_back();
                            t.push_front(QString().number(clsId));
                            t.push_front(QString().number(sizeId));
                            t.push_front(QString().number(colorId));
                            convertStream << t.join(" ") << "\n";
                        }
                        break;
                    }
                    case DataSet::NWPU: {
                        while (!ts.atEnd()) {
                            QString raw = ts.readLine();
                            QStringList t;
                            if (!StringProcess::processLabelString(raw, t)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            int colorId, clsId, sizeId;
                            if (!StringProcess::InitLabelInfo(
                                    t, colorId, clsId, sizeId, currentDataSet)) {
                                fail("格式错误!转换失败", labelPath);
                                continue;
                            }
                            switch (clsId) {
                            case 5: continue;
                            case 6:
                            case 7: clsId--; break;
                            }
                            t.pop_front();
                            t.push_front(QString().number(clsId));
                            t.push_front(QString().number(sizeId));
                            t.push_front(QString().number(colorId));
                            //<id> [目标各点的x、y归一化坐标]
                            convertStream << t.join(" ") << "\n";
                        }
                        break;
                    }
                    default: break;
                    }
                    convertStream.seek(0);
                    QString Text = convertStream.readAll();
                    buffer.close();
                    labelFile.close();
                    if (labelFile.open(QIODevice::WriteOnly)) {
                        ts << Text;
                        labelFile.close();
                    }
                }
            }
        } else {
            fail("导入失败!目标文件不存在!", fsModel_->filePath(target));
            return false;
        }
    }
    return true;
}
void FileService::tryOpenFirstAfterLoaded(const QString& dir) {
    if (!setProxyRoot(dir)) {
        emit busy(false);
        pendingDir_.clear();
        return;
    };
    // 优先：若指定了目标文件（比如恢复上次图片）
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
    }
    // 定位失败则退化为第一张（不清空 pendingTargetPath_）

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

// ---------- 工具方法 ----------
QModelIndex FileService::mapFromProxyToSource(const QModelIndex& p) const {
    if (!proxy_)
        return {};
    if (!p.isValid())
        return {};
    if (p.model() != proxy_) {
        qWarning() << "mapFromProxyToSource: index model mismatch";
        return {};
    }
    return static_cast<QSortFilterProxyModel*>(proxy_)->mapToSource(p);
}

QModelIndex FileService::mapFromSourceToProxy(const QModelIndex& s) const {
    if (!proxy_)
        return {};
    if (!s.isValid())
        return {};
    if (s.model() != fsModel_) {
        qWarning() << "mapFromSourceToProxy: index model mismatch";
        return {};
    }
    return static_cast<QSortFilterProxyModel*>(proxy_)->mapFromSource(s);
}
bool FileService::isImageFile(const QString& path) const {
    const QString low = path.toLower();
    for (const auto& ext : kImgExt)
        if (low.endsWith(ext.mid(1)))
            return true;
    return false;
}

void FileService::openPaths(const QStringList& paths) {
    if (paths.isEmpty())
        return;

    QString dir;
    pendingTargetPath_.clear();

    for (QString p : paths) {
        if (p.startsWith("file://")) {
            QUrl u(p);
            if (u.isLocalFile())
                p = u.toLocalFile();
        }
        QFileInfo fi(p);
        if (!fi.exists())
            continue;

        if (fi.isDir()) {
            dir = fi.absoluteFilePath();
            pendingTargetPath_.clear();
            break;
        } else if (fi.isFile()) {
            if (dir.isEmpty())
                dir = fi.absolutePath();
            if (pendingTargetPath_.isEmpty())
                pendingTargetPath_ = fi.absoluteFilePath();
        }
    }

    if (!dir.isEmpty()) {
        openDir(dir);
        QTimer::singleShot(0, this, [this, dir] { tryOpenFirstAfterLoaded(dir); });
    }
}

// ---------- 记忆 & 恢复 ----------
void FileService::saveLastVisited(const QString& imagePath) {
    // QSettings st("ATLabelMaster", "ATLabelMaster");
    // st.setValue("lastImagePath", imagePath);
    // st.setValue("lastDir", QFileInfo(imagePath).absolutePath());
    controller::AppSettings::instance().setlastImagePath(imagePath);
    controller::AppSettings::instance().setlastImageDir(QFileInfo(imagePath).absolutePath());
}

void FileService::tryRestoreLastVisited() {
    const QString lastImg = controller::AppSettings::instance().lastImagePath();
    const QString lastDir = controller::AppSettings::instance().lastImageDir();
    // QSettings st("ATLabelMaster", "ATLabelMaster");
    // const QString lastImg = st.value("lastImagePath").toString();
    // const QString lastDir = st.value("lastDir").toString();
    if (lastDir.isEmpty())
        return;

    if (!lastImg.isEmpty()) {
        pendingTargetPath_ = lastImg; // 先设目标，再 openDir
    }
    openDir(lastDir);
    // QTimer::singleShot(0, this, [this, lastDir] { tryOpenFirstAfterLoaded(lastDir); });
}

// ---------- 标注 I/O（归一化格式 + 兼容旧像素格式） ----------
QString FileService::labelFileForImage(const QString& imagePath) {
    QFileInfo fi(imagePath);
    QDir labelDir;
    if (controller::AppSettings::instance().saveDir().isEmpty()) {
        labelDir = QDir(fi.absolutePath() + "/../label");
    } else {
        // 绝对路径
        labelDir = QDir(controller::AppSettings::instance().saveDir());
        if (!labelDir.isAbsolute()) {
            // 相对路径
            labelDir = QDir(fi.absolutePath() + "/../" + labelDir.path());
        }
    }

    const QString dirPath = QDir::cleanPath(labelDir.absolutePath());
    return dirPath + "/" + fi.completeBaseName() + ".txt";
}

bool FileService::writeLabelFile(
    const QString& labelPath, const QVector<Armor>& armors, const QSize& imgSize) {
    if (imgSize.width() <= 0 || imgSize.height() <= 0)
        return false;

    QDir().mkpath(QFileInfo(labelPath).absolutePath());
    QFile f(labelPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream ts(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ts.setEncoding(QStringConverter::Utf8);
#else
    ts.setCodec("UTF-8");
#endif
    ts.setRealNumberNotation(QTextStream::FixedNotation);
    ts.setRealNumberPrecision(6); // 保留 6 位小数

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());
    auto norm      = [&](const QPointF& p) { return QPointF(p.x() / W, p.y() / H); };

    for (const auto& a : armors) {
        const int colorId = IdConvert::colorLetter2Id(a.color);
        const int classId = IdConvert::classToken2Id(
            IdConvert::normalizeClasslToken(
                a.cls));          // class size
                                  // classToken2IdCollection(normalizeClasslToken(a.cls),
                                  // idCollect); // 字符串

        const QPointF q0 = norm(a.p0), q1 = norm(a.p1), q2 = norm(a.p2), q3 = norm(a.p3);
        ts << colorId << ' ' << a.size << ' ' << classId << ' ' << q0.x() << ' ' << q0.y() << ' '
           << q1.x() << ' ' << q1.y() << ' ' << q2.x() << ' ' << q2.y() << ' ' << q3.x() << ' '
           << q3.y() << '\n';
    }
    return true;
}

QVector<Armor> FileService::readLabelFile(const QString& labelPath, const QSize& imgSize) {
    QVector<Armor> res;
    QFile f(labelPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return res;

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());

    QTextStream ts(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ts.setEncoding(QStringConverter::Utf8);
#else
    ts.setCodec("UTF-8");
#endif
    while (!ts.atEnd()) {
        QString raw = ts.readLine();
        int hash    = raw.indexOf('#');
        if (hash >= 0)
            raw = raw.left(hash);
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;

        // color size label x1 y1 x2 y2 x3 y3 x4 y4
        const QStringList t = line.simplified().split(' ');
        if (t.size() != 11)
            continue;

        bool ok  = true;
        auto tod = [&](int i) -> double {
            bool o   = false;
            double v = t.at(i).toDouble(&o);
            ok &= o;
            return v;
        };

        Armor a;
        // 颜色字段：兼容“数字"
        bool okInt  = false;
        int colorId = t.at(0).toInt(&okInt);
        int size    = t.at(1).toInt(&okInt);
        int classId = t.at(2).toInt(&okInt);
        if (!okInt) {
            continue;
        }
        a.color   = IdConvert::colorId2Letter(colorId);
        a.cls     = IdConvert::idCollect2Token(classId);
        a.size    = size;
        a.score   = 0.f;
        double x0 = tod(3), y0 = tod(4), x1 = tod(5), y1 = tod(6), x2 = tod(7), y2 = tod(8),
               x3 = tod(9), y3 = tod(10);
        if (!ok)
            continue;

        // 归一化判定：坐标绝对值的最大值 <= 1.5 视为已归一化（留容错）
        const double mx = std::max({std::fabs(x0), std::fabs(x1), std::fabs(x2), std::fabs(x3)});
        const double my = std::max({std::fabs(y0), std::fabs(y1), std::fabs(y2), std::fabs(y3)});
        const bool normalized = (mx <= 1.5 && my <= 1.5 && W > 0 && H > 0);

        auto denorm = [&](double x, double y) -> QPointF {
            return normalized ? QPointF(x * W, y * H) : QPointF(x, y);
        };

        a.p0 = denorm(x0, y0);
        a.p1 = denorm(x1, y1);
        a.p2 = denorm(x2, y2);
        a.p3 = denorm(x3, y3);

        res.push_back(a);
    }
    return res;
}

// ---------- 保存标注（对外槽） ----------
void FileService::saveData(const QVector<Armor>& armors, const QImage& image, bool needSaveImg) {
    if (!pendingDir_.isEmpty()) {
        emit status(tr("目录加载中，稍后保存"), 900);
        return;
    }

    QString imgPath = currentImagePath_;
    if (imgPath.isEmpty()) {
        if (!proxyCurrent_.isValid() || proxyCurrent_.model() != proxy_) {
            emit status(tr("未选中图片"), 900);
            return;
        }
        const QModelIndex s = mapFromProxyToSource(proxyCurrent_);
        if (!s.isValid() || fsModel_->isDir(s)) {
            emit status(tr("未选中图片"), 900);
            return;
        }
        imgPath = fsModel_->filePath(s);
    }

    // 获取图片尺寸（优先用已缓存尺寸；为空则从文件探测）
    QSize sz = currentImageSize_;
    if (sz.isEmpty()) {
        QImageReader rr(imgPath);
        sz = rr.size();
        if (sz.isEmpty()) {
            emit status(tr("无法获取图片尺寸"), 1200);
            return;
        }
    }
    // 保存图片
    if (needSaveImg) {
        if (image.save(imgPath)) {
            emit status(tr("已保存图片：%1").arg(QFileInfo(imgPath).fileName()), 900);
            LOGI(QString("保存图片：%1").arg(imgPath));
        } else {
            emit status(tr("保存图片失败"), 1200);
            LOGE(QString("保存图片失败：%1").arg(imgPath));
        }
    }
    // 保存标注
    QString lblPath = labelFileForImage(imgPath);
    if (writeLabelFile(lblPath, armors, sz)) {
        emit status(tr("已保存标注：%1").arg(QFileInfo(lblPath).fileName()), 900);
        LOGI(QString("保存标注：%1").arg(lblPath));
    } else {
        emit status(tr("保存失败"), 1200);
        LOGE(QString("保存失败：%1").arg(lblPath));
    }
}
// 获取统计数据
void FileService::getStas(int colorId, int classId, int sizeId) {
    // 开始统计
    int targetCount    = 0;
    int fileCount      = 0;
    QModelIndex parent = proxyCurrent_.parent();
    for (int i = 0; i < proxy_->rowCount(parent); i++) {
        int hasTarget   = false;
        QString imgPath = fsModel_->filePath(fsModel_->index(i, 0, mapFromProxyToSource(parent)));
        const QString labelPath = labelFileForImage(imgPath);
        if (QFile::exists(labelPath)) {
            QFile file(labelPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream ts(&file);
                while (!ts.atEnd()) {
                    QStringList t;
                    if (!StringProcess::processLabelString(ts.readLine(), t)) {
                        continue;
                    }
                    bool ok   = false;
                    int colId = t.at(0).toInt(&ok);
                    int sId   = t.at(1).toInt(&ok);
                    int clsId = t.at(2).toInt(&ok);
                    if (!ok) {
                        continue;
                    }
                    auto checkId = [&](const int& value, const int& target) {
                        if (target == -1) {
                            return true;
                        } else {
                            if (value == target) {
                                return true;
                            }
                        }
                        return false;
                    };
                    if (checkId(colId, colorId) && checkId(sId, sizeId)
                        && checkId(clsId, classId)) {
                        targetCount++;
                        hasTarget = true;
                    }
                }
                if (hasTarget) {
                    fileCount++;
                }
            }
        }
    }
    emit StasGetted(targetCount, fileCount);
}
