// ===============================
// File: service/file.cpp
// ===============================
#include "service/file.hpp"

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# include <QStringConverter> // Qt6: QTextStream::setEncoding
#endif
#include <QTextStream>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <cmath>

#include "controller/dataset.hpp"
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

// ---------- 工具：token 规范化 ----------
QString FileService::colorToToken(const QString& letter) {
    const QString L = letter.trimmed().left(1).toUpper();
    if (L == "B")
        return "BLUE";
    if (L == "R")
        return "RED";
    if (L == "G")
        return "GRAY";
    if (L == "P")
        return "PURPLE";
    const QString U = letter.trimmed().toUpper();
    if (U == "BLUE" || U == "RED" || U == "GRAY" || U == "PURPLE")
        return U;
    return "GRAY";
}
QString FileService::letterFromColorToken(const QString& tk) {
    const QString U = tk.trimmed().toUpper();
    if (U == "BLUE")
        return "B";
    if (U == "RED")
        return "R";
    if (U == "GRAY")
        return "G";
    if (U == "PURPLE")
        return "P";
    if (U == "B" || U == "R" || U == "G" || U == "P")
        return U;
    return "G";
}
QString FileService::normalizeLabelToken(const QString& cls) {
    const QString s = cls.trimmed();
    QString u       = s.toUpper();
    if (u == "G")
        return "G";
    if (u == "O")
        return "O";
    if (u == "BS")
        return "Bs";
    if (u == "BB")
        return "Bb";
    if (u == "1" || u == "2" || u == "3" || u == "4")
        return u;
    if (s == "Bs" || s == "Bb")
        return s;
    return s;
}

// 颜色字母(B/R/G/P) → id(0/1/2/3)
int FileService::colorIdFromLetter(const QString& letter) {
    const QChar c = letter.trimmed().isEmpty() ? QChar() : letter.trimmed().at(0).toUpper();
    if (c == 'B')
        return 0;       // BLUE
    if (c == 'R')
        return 1;       // RED
    if (c == 'G')
        return 2;       // GRAY
    if (c == 'P')
        return 3;       // PURPLE
    const QString u = letter.trimmed().toUpper();
    if (u == "BLUE")
        return 0;
    if (u == "RED")
        return 1;
    if (u == "GRAY")
        return 2;
    if (u == "PURPLE")
        return 3;
    return 2;           // 默认 GRAY
}
QString FileService::letterFromColorId(int id) {
    switch (id) {
    case 0: return "B"; // BLUE
    case 1: return "R"; // RED
    case 2: return "G"; // GRAY
    case 3: return "P"; // PURPLE
    default: return "G";
    }
}

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

    // 目录加载完成后再尝试选第一张
    connect(
        fsModel_, &QFileSystemModel::directoryLoaded, this,
        [this](const QString& path) {
            if (pendingDir_.isEmpty())
                return;
            if (path == pendingDir_ || path.startsWith(pendingDir_ + '/')) {
                tryOpenFirstAfterLoaded(pendingDir_);
            }
        },
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

// ---------- 打开入口 ----------
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

    currentImagePath_ = path;       // 记住路径（保存时用）
    currentImageSize_ = img.size(); // 记住尺寸（保存/反归一化）
    saveLastVisited(path);

    controller::DatasetManager::instance().saveProgress(/*index=*/0);

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
void FileService::next() {
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
        LOGW(QString("已删除：%1").arg(path));
        next();
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

    pendingDir_               = dir; // 不清空 pendingTargetPath_，以便恢复时指定目标文件
    const QModelIndex srcRoot = fsModel_->setRootPath(dir); // 异步开始
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

    controller::AppSettings::instance().setlastImageDir(dir);
    controller::DatasetManager::instance().setImageDir(dir);

    tryOpenFirstAfterLoaded(dir);
    return true;
}

void FileService::tryOpenFirstAfterLoaded(const QString& dir) {
    if (!fsModel_ || !proxy_)
        return;

    QModelIndex srcRoot = fsModel_->index(dir);
    if (!srcRoot.isValid())
        return;

    QModelIndex pxRoot = mapFromSourceToProxy(srcRoot);
    if (!pxRoot.isValid())
        return;

    if (pxRoot.model() != proxy_)
        return;

    proxyRoot_ = pxRoot;

    const int rows = proxy_->rowCount(proxyRoot_);
    if (rows == 0)
        return;

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
                return;
            }
        }
        // 定位失败则退化为第一张（不清空 pendingTargetPath_）
    }

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
    QSettings st("ATLabelMaster", "ATLabelMaster");
    st.setValue("lastImagePath", imagePath);
    st.setValue("lastDir", QFileInfo(imagePath).absolutePath());
}

void FileService::tryRestoreLastVisited() {
    QSettings st("ATLabelMaster", "ATLabelMaster");
    const QString lastImg = st.value("lastImagePath").toString();
    const QString lastDir = st.value("lastDir").toString();
    if (lastDir.isEmpty())
        return;

    if (!lastImg.isEmpty()) {
        pendingTargetPath_ = lastImg; // 先设目标，再 openDir
    }
    openDir(lastDir);
    QTimer::singleShot(0, this, [this, lastDir] { tryOpenFirstAfterLoaded(lastDir); });
}

// ---------- 标注 I/O（归一化格式 + 兼容旧像素格式） ----------
QString FileService::labelFileForImage(const QString& imagePath) {
    QFileInfo fi(imagePath);
    QDir labelDir(fi.absolutePath() + "/../label");
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
    ts.setRealNumberPrecision(6);                           // 保留 6 位小数

    const double W = double(imgSize.width());
    const double H = double(imgSize.height());
    auto norm      = [&](const QPointF& p) { return QPointF(p.x() / W, p.y() / H); };

    for (const auto& a : armors) {
        const int colorId     = colorIdFromLetter(a.color); // 0/1/2/3
        const QString labelTk = normalizeLabelToken(a.cls); // 字符串

        const QPointF q0 = norm(a.p0), q1 = norm(a.p1), q2 = norm(a.p2), q3 = norm(a.p3);
        ts << colorId << ' ' << labelTk << ' ' << q0.x() << ' ' << q0.y() << ' ' << q1.x() << ' '
           << q1.y() << ' ' << q2.x() << ' ' << q2.y() << ' ' << q3.x() << ' ' << q3.y() << '\n';
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

        // color label x1 y1 x2 y2 x3 y3 x4 y4
        const QStringList t = line.simplified().split(' ');
        if (t.size() != 10)
            continue;

        bool ok  = true;
        auto tod = [&](int i) -> double {
            bool o   = false;
            double v = t.at(i).toDouble(&o);
            ok &= o;
            return v;
        };

        Armor a;
        // 颜色字段：兼容“数字或字符串”
        bool okInt = false;
        int cid    = t.at(0).toInt(&okInt);
        a.color    = okInt ? letterFromColorId(cid)         // 数字 → 字母
                           : letterFromColorToken(t.at(0)); // 字符串 → 字母

        a.cls   = normalizeLabelToken(t.at(1));
        a.score = 0.f;

        double x0 = tod(2), y0 = tod(3), x1 = tod(4), y1 = tod(5), x2 = tod(6), y2 = tod(7),
               x3 = tod(8), y3 = tod(9);
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
void FileService::saveLabels(const QVector<Armor>& armors) {
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

    const QString lblPath = labelFileForImage(imgPath);
    if (writeLabelFile(lblPath, armors, sz)) {
        emit status(tr("已保存标注：%1").arg(QFileInfo(lblPath).fileName()), 900);
        LOGI(QString("保存标注：%1").arg(lblPath));
    } else {
        emit status(tr("保存失败"), 1200);
        LOGE(QString("保存失败：%1").arg(lblPath));
    }
}
