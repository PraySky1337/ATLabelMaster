#pragma once
#include "service/file.hpp"
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
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <cmath>
#include <cstddef>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <qabstractitemmodel.h>
#include <qbuffer.h>
#include <qdebug.h>
#include <qdir.h>
#include <qglobal.h>
#include <qhashfunctions.h>
#include <qiodevicebase.h>
#include <qlist.h>
#include <qlocale.h>
#include <qmath.h>
#include <qnamespace.h>
#include <qpointer.h>
#include <qsettings.h>
#include <qsortfilterproxymodel.h>
#include <qstringalgorithms.h>
#include <qtmetamacros.h>
namespace StringProcess {
inline bool processLabelString(QString raw, QStringList& result) {
    int hash = raw.indexOf('#');
    if (hash >= 0)
        raw = raw.left(hash);
    const QString line = raw.trimmed();
    if (line.isEmpty())
        return false;
    const QStringList list = line.simplified().split(' ');
    result                 = list;
    return true;
}
inline bool InitLabelInfo(
    const QStringList& label, int& colorId, int& classId, int& sizeId, DataSet dataset) {
    const int colorCounts = 4;
    int corLabelSize;
    int classCounts;
    int posStart = 2;
    sizeId       = 0;
    bool ok      = true;
    switch (dataset) {
    case DataSet::LabelMaster:
        colorId = label[0].toInt(&ok);
        classId = label[1].toInt(&ok);
        if (classId == 1) {
            sizeId = 1;
        }
        posStart     = 2;
        classCounts  = 8;
        corLabelSize = 10;
        break;
    case DataSet::LabelMaster2:
        colorId      = label[0].toInt(&ok);
        sizeId       = label[1].toInt(&ok);
        classId      = label[2].toInt(&ok);
        posStart     = 3;
        classCounts  = 8;
        corLabelSize = 11;
        break;
    case DataSet::LabelMaster3:
        // 新格式: color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3 (15字段)
        colorId      = label[0].toInt(&ok);
        sizeId       = label[1].toInt(&ok);
        classId      = label[2].toInt(&ok);
        // 字段 3-6: x, y, w, h (边界矩形)
        // 字段 7-14: x0, y0, x1, y1, x2, y2, x3, y3 (角点)
        posStart     = 7;
        classCounts  = 8;
        corLabelSize = 15;  // 15字段
        break;
    case DataSet::HITSZ:
        colorId = label[label.size() - 1].toInt(&ok);
        classId = label[label.size() - 2].toInt(&ok);
        switch (classId) {
        case 1:
        case 8:
        case 9:
        case 10:
        case 11: sizeId = 1;
        }
        posStart     = 0;
        classCounts  = 12;
        corLabelSize = 10;
        break;
    case DataSet::UPC:
        colorId = label[0].toInt(&ok);
        classId = label[1].toInt(&ok);
        switch (classId) {
        case 1:
        case 8:
        case 9:
        case 10:
        case 11: sizeId = 1;
        }
        posStart     = 2;
        classCounts  = 12;
        corLabelSize = 10;
        break;
    case DataSet::NWPU: {
        int normalizedId = label[0].toInt(&ok);
        colorId          = normalizedId / 16;
        int remain       = normalizedId % 16;
        sizeId           = remain / 8;
        classId          = remain % 8;
        posStart         = 1;
        classCounts      = 8;
        corLabelSize     = 9;

    }

    break;
    default: return false;
    }
    if (!ok) {
        return false;
    }
    for (int i = posStart; i < posStart + 8; i++) {
        if (std::fabs(label.at(i).toDouble(&ok)) > 1.5) {
            return false;
        }
    }
    if (corLabelSize != label.size() || (classId < 0 && classId >= classCounts)
        || (colorId < 0 && colorId >= colorCounts) || sizeId > 1 || sizeId < 0) {
        return false;
    }
    return true;
};

} // namespace StringProcess