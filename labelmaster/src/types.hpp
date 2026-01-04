#pragma once
#include <QMetaType>
#include <QPointF>
#include <QString>
#include <QVector>

struct Armor {
    QString cls;
    QString color;
    float score = 0.f;
    // 角点顺序：从 0 开始逆时针：TL(0) → BL(1) → BR(2) → TR(3)，全为“原图坐标”
    QPointF p0, p1, p2, p3;
    QPointF norm_p0, norm_p1, norm_p2, norm_p3;
    int size = false; // small 0 big 1
};

Q_DECLARE_METATYPE(Armor)
Q_DECLARE_METATYPE(QVector<Armor>)
