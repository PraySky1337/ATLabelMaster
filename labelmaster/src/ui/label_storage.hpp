#pragma once
#include <QRect>
#include <QString>

namespace ui {

// 导出裁剪得到的 patch，返回 "不带路径" 的文件名（用于派生 labels 文件名）
QString exportPatch(const QString& imagePath, const QRect& roiImgPx, const QString& saveDir);

// 以 “整幅 patch 覆盖全图” 的假设，写 YOLO 标签到 <saveDir>/labels/<patchName>.txt
bool saveYoloLabel(const QString& patchFileName, const QString& clsName, const QString& saveDir);

// ⭐ 新增：带 ROI 的便捷重载（先切 patch，再为该 patch 写整框标签）
bool saveYoloLabel(
    const QString& imagePath, const QRect& roiImgPx, const QString& clsName,
    const QString& saveDir);

} // namespace ui
