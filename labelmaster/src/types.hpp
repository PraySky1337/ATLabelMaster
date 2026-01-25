#pragma once
#include <QMetaType>
#include <QPointF>
#include <QString>
#include <QVector>

struct Armor {
    QString cls;
    QString color;
    float score = 0.f;
    // 角点顺序：从 0 开始逆时针：TL(0) → BL(1) → BR(2) → TR(3)，全为"原图坐标"
    QPointF p0, p1, p2, p3;
    QPointF norm_p0, norm_p1, norm_p2, norm_p3;
    int size = false; // small 0 big 1

    // 新增: 归一化边界矩形 (用于15字段格式)
    double norm_x = -1;  // center_x (归一化), -1 表示未设置
    double norm_y = -1;  // center_y
    double norm_w = -1;  // width
    double norm_h = -1;  // height
};

Q_DECLARE_METATYPE(Armor)
Q_DECLARE_METATYPE(QVector<Armor>)
