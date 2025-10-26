#include "label_storage.hpp"
#include "classes.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTextStream>

namespace ui {

static QString ensureSubdir(const QString& root, const char* sub) {
    QDir d(root);
    if (!d.exists())
        d.mkpath(".");
    if (!d.exists(sub))
        d.mkpath(sub);
    return d.filePath(sub);
}

QString exportPatch(const QString& imagePath, const QRect& roiImgPx, const QString& saveDir) {
    QImage img(imagePath);
    if (img.isNull() || roiImgPx.isNull())
        return {};

    QRect clipped = roiImgPx.intersected(QRect(0, 0, img.width(), img.height()));
    if (clipped.isNull())
        return {};

    QImage patch = img.copy(clipped);

    QFileInfo fi(imagePath);
    QString base = fi.completeBaseName();
    QString ext  = fi.suffix().isEmpty() ? "png" : fi.suffix().toLower();

    // 文件名包含 ROI 信息，便于回溯
    QString fname = QString("%1_x%2_y%3_w%4_h%5.%6")
                        .arg(base)
                        .arg(clipped.x())
                        .arg(clipped.y())
                        .arg(clipped.width())
                        .arg(clipped.height())
                        .arg(ext);

    QString imagesDir = ensureSubdir(saveDir, "images");
    QString outPath   = QDir(imagesDir).filePath(fname);

    if (!patch.save(outPath))
        return {};
    return fname; // 不含路径，供 label 使用
}

bool saveYoloLabel(const QString& patchFileName, const QString& clsName, const QString& saveDir) {
    int cid = classes::idOf(clsName);

    // YOLO：x y w h（归一化） 这里覆盖全图
    const double xc = 0.5, yc = 0.5, w = 1.0, h = 1.0;

    QString labelsDir = ensureSubdir(saveDir, "labels");
    QString base      = patchFileName;
    int dot           = base.lastIndexOf('.');
    if (dot > 0)
        base = base.left(dot);
    QString labelPath = QDir(labelsDir).filePath(base + ".txt");

    QFile f(labelPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    QTextStream os(&f);
    os.setRealNumberNotation(QTextStream::FixedNotation);
    os.setRealNumberPrecision(6);
    os << cid << " " << xc << " " << yc << " " << w << " " << h << "\n";
    f.close();
    return true;
}

bool saveYoloLabel(
    const QString& imagePath, const QRect& roiImgPx, const QString& clsName,
    const QString& saveDir) {
    // 1) 先切 ROI 出 patch
    const QString patchFileName = exportPatch(imagePath, roiImgPx, saveDir);
    if (patchFileName.isEmpty())
        return false;

    // 2) 为该 patch 写 YOLO 标签（整框覆盖）
    return saveYoloLabel(patchFileName, clsName, saveDir);
}

} // namespace ui
