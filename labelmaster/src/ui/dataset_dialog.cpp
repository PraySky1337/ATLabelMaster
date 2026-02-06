#include "dataset_dialog.hpp"
#include "controller/settings.hpp"
#include "util/id_convert.hpp"
#include "util/svg_constants.hpp"

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVariant>
#include <QWidget>
#include <QTransform>
#include <QPolygonF>
#include <limits>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QTextStream>

using namespace ui;
using namespace labelmaster::ui;

DatasetDialog::DatasetDialog(QWidget* parent)
    : PixelDialog(parent)
    , ui_(new Ui::DatasetDialog) {
    ui_->setupUi(this);
    this->setWindowTitle("数据集管理");

    // Initialize import format combo
    ui_->import_format_combo->setCurrentIndex(controller::AppSettings::instance().importFormat());
    connect(ui_->import_format_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DatasetDialog::setimportFormat);

    // Initialize output format combo
    ui_->format_combo->setCurrentIndex(controller::AppSettings::instance().outputFormat());
    connect(ui_->format_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        controller::AppSettings::instance().setoutputFormat(index);
    });

    // Connect batch operations buttons
    connect(ui_->batch_replace_button, &QPushButton::clicked,
            this, &DatasetDialog::performBatchReplace);
    connect(ui_->batch_convert_button, &QPushButton::clicked,
            this, &DatasetDialog::performBatchConvert);
}

DatasetDialog::~DatasetDialog() = default;

// ---------- Helper Functions ----------
DatasetDialog::LabelInfo DatasetDialog::getLabelFromCombos(
    QComboBox* colorCombo, QComboBox* sizeCombo, QComboBox* classCombo) const {
    LabelInfo info;

    // 颜色: All(-1), Blue(0), Red(1), Gray(2), Purple(3)
    QString colorText = colorCombo->currentText();
    if (colorText == "All") {
        info.colorId = -1;  // -1 表示匹配所有颜色
    } else {
        info.colorId = IdConvert::colorToken2Id(colorText);
    }

    // 大小: All(-1), Small(0), Big(1)
    QString sizeText = sizeCombo->currentText();
    if (sizeText == "All") {
        info.size = -1;  // -1 表示匹配所有大小
    } else {
        info.size = (sizeText == "Small") ? 0 : 1;
    }

    // 类别: G(0), 1-5(1-5), O(6), B(7)
    QString classText = classCombo->currentText();
    info.classId = IdConvert::classToken2Id(IdConvert::normalizeClasslToken(classText));

    return info;
}

// ---------- Import Format ----------
void DatasetDialog::setimportFormat(int index) {
    controller::AppSettings::instance().setimportFormat(index);
    update();
}

// ---------- Batch Replace ----------
void DatasetDialog::performBatchReplace() {
    // 获取源和目标标签
    LabelInfo src = getLabelFromCombos(
        ui_->src_color_combo, ui_->src_size_combo, ui_->src_class_combo);
    LabelInfo dst = getLabelFromCombos(
        ui_->dst_color_combo, ui_->dst_size_combo, ui_->dst_class_combo);

    // 获取标签保存目录
    QString labelDir = controller::AppSettings::instance().saveDir();
    if (labelDir.isEmpty()) {
        ui_->batch_result_text->setPlainText("错误: 请先设置标签保存目录");
        return;
    }

    // 处理相对路径 - 将相对路径转换为绝对路径
    QDir dir(labelDir);
    if (!dir.isAbsolute()) {
        // 如果是相对路径，尝试基于当前目录转换
        QString currentDir = QDir::currentPath();
        QString absPath = QDir::cleanPath(currentDir + "/" + labelDir);
        dir = QDir(absPath);

        // 如果目录仍然不存在，尝试基于上次图片目录
        if (!dir.exists()) {
            QString lastImgDir = controller::AppSettings::instance().lastImageDir();
            if (!lastImgDir.isEmpty()) {
                QFileInfo imgFi(lastImgDir);
                if (imgFi.dir().exists()) {
                    absPath = QDir::cleanPath(imgFi.absolutePath() + "/../" + labelDir);
                    dir = QDir(absPath);
                }
            }
        }
    }

    if (!dir.exists()) {
        ui_->batch_result_text->setPlainText(
            "错误: 标签目录不存在: " + labelDir + "\n"
            "解析路径为: " + dir.absolutePath() + "\n"
            "请在设置中配置保存目录。");
        return;
    }

    // 获取所有 .txt 标签文件
    QStringList filters;
    filters << "*.txt";
    dir.setNameFilters(filters);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::Readable);

    int totalFiles = 0;
    int modifiedFiles = 0;
    int replacedLabels = 0;
    QStringList modifiedFileNames;

    for (const QFileInfo& fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        totalFiles++;
        QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        in.setEncoding(QStringConverter::Utf8);
#else
        in.setCodec("UTF-8");
#endif

        QStringList newLines;
        bool fileModified = false;
        int labelsReplacedInFile = 0;

        while (!in.atEnd()) {
            QString line = in.readLine();
            int hashIdx = line.indexOf('#');
            if (hashIdx >= 0) {
                // Keep comments as-is
                if (hashIdx == 0) {
                    newLines.append(line);
                    continue;
                }
                line = line.left(hashIdx);
            }

            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) {
                newLines.append(line);
                continue;
            }

            // Parse: 支持11字段和15字段格式
            // 11字段: color size class x0 y0 x1 y1 x2 y2 x3 y3
            // 15字段: color size class x y w h x0 y0 x1 y1 x2 y2 x3 y3
            QStringList parts = trimmed.simplified().split(' ');
            if (parts.size() != 11 && parts.size() != 15) {
                newLines.append(line);
                continue;
            }

            bool ok = true;
            int colorId = parts[0].toInt(&ok);
            int size = parts[1].toInt(&ok);
            int classId = parts[2].toInt(&ok);

            if (!ok) {
                newLines.append(line);
                continue;
            }

            // 检查是否匹配源标签 (-1 表示通配符，匹配任意值)
            bool colorMatch = (src.colorId == -1 || colorId == src.colorId);
            bool sizeMatch = (src.size == -1 || size == src.size);
            bool classMatch = (classId == src.classId);

            if (colorMatch && sizeMatch && classMatch) {
                // 替换为目标标签
                int dstColorId = (dst.colorId == -1) ? colorId : dst.colorId;
                int dstSize = (dst.size == -1) ? size : dst.size;

                // 根据原始格式输出
                if (parts.size() == 11) {
                    // 11字段格式: color size class x0 y0 x1 y1 x2 y2 x3 y3
                    line = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11")
                        .arg(dstColorId)
                        .arg(dstSize)
                        .arg(dst.classId)
                        .arg(parts[3]).arg(parts[4]).arg(parts[5]).arg(parts[6])
                        .arg(parts[7]).arg(parts[8]).arg(parts[9]).arg(parts[10]);
                } else {
                    // 15字段格式: color size class x y w h x0 y0 x1 y1 x2 y2 x3 y3
                    line = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15")
                        .arg(dstColorId)
                        .arg(dstSize)
                        .arg(dst.classId)
                        .arg(parts[3]).arg(parts[4]).arg(parts[5]).arg(parts[6])  // x y w h
                        .arg(parts[7]).arg(parts[8]).arg(parts[9]).arg(parts[10]) // x0 y0 x1 y1
                        .arg(parts[11]).arg(parts[12]).arg(parts[13]).arg(parts[14]); // x2 y2 x3 y3
                }
                fileModified = true;
                labelsReplacedInFile++;
                replacedLabels++;
            }

            newLines.append(line);
        }

        // 如果文件被修改，写回
        if (fileModified) {
            QFile writeFile(filePath);
            if (writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream out(&writeFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                out.setEncoding(QStringConverter::Utf8);
#else
                out.setCodec("UTF-8");
#endif
                for (const QString& newLine : newLines) {
                    out << newLine << '\n';
                }
                modifiedFiles++;
                modifiedFileNames.append(fileInfo.fileName());
            } else {
                qWarning() << "Failed to open file for writing:" << filePath;
            }
        }
    }

    // 生成统计报告
    QString srcDesc = QString("颜色=%1, 大小=%2, 类别=%3")
        .arg(ui_->src_color_combo->currentText())
        .arg(ui_->src_size_combo->currentText())
        .arg(ui_->src_class_combo->currentText());
    QString dstDesc = QString("颜色=%1, 大小=%2, 类别=%3")
        .arg(ui_->dst_color_combo->currentText())
        .arg(ui_->dst_size_combo->currentText())
        .arg(ui_->dst_class_combo->currentText());

    QString report = QString("=== 批量替换统计 ===\n\n")
        + QString("标签目录: %1\n\n").arg(dir.absolutePath())
        + QString("替换规则:\n")
        + QString("  源: %1\n").arg(srcDesc)
        + QString("  目标: %1\n\n").arg(dstDesc)
        + QString("扫描文件数: %1\n").arg(totalFiles)
        + QString("修改文件数: %1\n").arg(modifiedFiles)
        + QString("替换标签数: %1\n\n").arg(replacedLabels);

    if (!modifiedFileNames.isEmpty() && modifiedFileNames.size() <= 50) {
        report += "已修改的文件:\n";
        for (const QString& name : modifiedFileNames) {
            report += "  - " + name + "\n";
        }
    } else if (modifiedFileNames.size() > 50) {
        report += QString("已修改的文件: %1 个 (列表已省略)\n").arg(modifiedFileNames.size());
    }

    ui_->batch_result_text->setPlainText(report);
}

// ---------- Batch Convert ----------
void DatasetDialog::performBatchConvert() {
    // 目标格式 = 当前选择的格式（0=11字段, 1=15字段）
    int targetFormat = controller::AppSettings::instance().outputFormat();
    QString labelDir = controller::AppSettings::instance().saveDir();

    // 确认对话框
    QString formatName = (targetFormat == 1) ? "Rect + Points (15 fields)" : "Points Only (11 fields)";
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认转换",
        QString("将当前目录所有标签转换为:\n%1\n\n继续?").arg(formatName),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) return;

    // 获取标签目录
    QDir dir(labelDir);
    if (!dir.isAbsolute()) {
        QString lastImgDir = controller::AppSettings::instance().lastImageDir();
        if (!lastImgDir.isEmpty()) {
            dir = QDir(QDir::cleanPath(QFileInfo(lastImgDir).absolutePath() + "/../" + labelDir));
        }
    }

    QStringList filters; filters << "*.txt";
    dir.setNameFilters(filters);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::Readable);

    int successCount = 0, skipCount = 0, failCount = 0;

    for (const QFileInfo& fileInfo : fileList) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) { failCount++; continue; }

        QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        in.setEncoding(QStringConverter::Utf8);
#else
        in.setCodec("UTF-8");
#endif

        QStringList newLines;
        bool fileModified = false;
        QString line;

        while (!in.atEnd()) {
            line = in.readLine();
            int hashIdx = line.indexOf('#');
            if (hashIdx >= 0) {
                if (hashIdx == 0) { newLines.append(line); continue; }
                line = line.left(hashIdx);
            }

            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) { newLines.append(line); continue; }

            QStringList parts = trimmed.simplified().split(' ');

            // 判断当前行的格式
            bool isCurrent11 = (parts.size() == 11);
            bool isCurrent15 = (parts.size() == 15);

            // 如果目标是15字段，且当前是11字段 → 转换
            if (targetFormat == 1 && isCurrent11) {
                // 11字段 → 15字段
                // color size cls x0 y0 x1 y1 x2 y2 x3 y3
                // → color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3

                bool ok = true;
                auto tod = [&](int i) -> double {
                    bool o = false;
                    double v = parts[i].toDouble(&o);
                    ok &= o;
                    return v;
                };
                if (ok) {
                    // 读取基本信息
                    int size = parts[1].toInt();

                    // 读取四个锚点（归一化坐标）
                    QPointF p0(tod(3), tod(4));  // TL
                    QPointF p1(tod(5), tod(6));  // BL
                    QPointF p2(tod(7), tod(8));  // BR
                    QPointF p3(tod(9), tod(10)); // TR

                    // 获取SVG模板
                    const auto& svgTemplate = (size == 0)
                        ? labelmaster::util::SvgConstants::smallArmor()
                        : labelmaster::util::SvgConstants::bigArmor();

                    // SVG四边形（四个角点）
                    QPolygonF svg_quad;
                    svg_quad << QPointF(0., 0.)
                             << QPointF(0., svgTemplate.height)
                             << QPointF(svgTemplate.width, svgTemplate.height)
                             << QPointF(svgTemplate.width, 0.);

                    // 图像锚点（归一化坐标）
                    QPolygonF img_anchors;
                    img_anchors << p0 << p1 << p2 << p3;

                    // 透视变换
                    QTransform transform;
                    double x, y, w, h;

                    if (QTransform::quadToQuad(svgTemplate.anchors, img_anchors, transform)) {
                        // 将SVG四边形变换到图像空间
                        QPolygonF img_corners = transform.map(svg_quad);

                        // 计算边界框（最小外接矩形）
                        double min_x = std::numeric_limits<double>::max();
                        double min_y = std::numeric_limits<double>::max();
                        double max_x = std::numeric_limits<double>::lowest();
                        double max_y = std::numeric_limits<double>::lowest();

                        for (const auto& pt : img_corners) {
                            min_x = std::min(min_x, pt.x());
                            min_y = std::min(min_y, pt.y());
                            max_x = std::max(max_x, pt.x());
                            max_y = std::max(max_y, pt.y());
                        }

                        // 计算中心点和尺寸（已经是归一化坐标）
                        w = max_x - min_x;
                        h = max_y - min_y;
                        x = (min_x + max_x) / 2.0;
                        y = (min_y + max_y) / 2.0;

                        // Clamp to [0,1]
                        auto clamp01 = [](double v) { return std::clamp(v, 0.0, 1.0); };
                        x = clamp01(x);
                        y = clamp01(y);
                        w = clamp01(w);
                        h = clamp01(h);
                    } else {
                        // 透视变换失败时的fallback：使用简单的锚点边界框
                        double min_x = std::min({p0.x(), p1.x(), p2.x(), p3.x()});
                        double min_y = std::min({p0.y(), p1.y(), p2.y(), p3.y()});
                        double max_x = std::max({p0.x(), p1.x(), p2.x(), p3.x()});
                        double max_y = std::max({p0.y(), p1.y(), p2.y(), p3.y()});

                        w = max_x - min_x;
                        h = max_y - min_y;
                        x = (min_x + max_x) / 2.0;
                        y = (min_y + max_y) / 2.0;
                    }

                    // 输出: color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3
                    line = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15")
                        .arg(parts[0]).arg(parts[1]).arg(parts[2])
                        .arg(x).arg(y).arg(w).arg(h)
                        .arg(parts[3]).arg(parts[4])  // x0, y0
                        .arg(parts[5]).arg(parts[6])  // x1, y1
                        .arg(parts[7]).arg(parts[8])  // x2, y2
                        .arg(parts[9]).arg(parts[10]); // x3, y3
                    fileModified = true;
                }
            }
            // 如果目标是11字段，且当前是15字段 → 转换
            else if (targetFormat == 0 && isCurrent15) {
                // 15字段 → 11字段
                // 格式: color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3
                //       → color size cls x0 y0 x1 y1 x2 y2 x3 y3
                // 保留SVG锚点 (x0,y0=TL, x1,y1=BL, x2,y2=BR, x3,y3=TR)
                // 丢弃bbox信息 (x y w h)
                line = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11")
                    .arg(parts[0]).arg(parts[1]).arg(parts[2])
                    .arg(parts[7]).arg(parts[8])   // x0, y0 (TL)
                    .arg(parts[9]).arg(parts[10])  // x1, y1 (BL)
                    .arg(parts[11]).arg(parts[12]) // x2, y2 (BR)
                    .arg(parts[13]).arg(parts[14]); // x3, y3 (TR)
                fileModified = true;
            }
            // 如果已经是目标格式，保持不变
            newLines.append(line);
        }

        if (fileModified) {
            file.resize(0);
            QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            out.setEncoding(QStringConverter::Utf8);
#else
            out.setCodec("UTF-8");
#endif
            for (const QString& newLine : newLines) { out << newLine << '\n'; }
            successCount++;
        } else {
            skipCount++;
        }
        file.close();
    }

    QMessageBox::information(this, "转换完成",
        QString("转换成功: %1 个文件\n已跳过: %2 个文件\n转换失败: %3 个文件").arg(successCount).arg(skipCount).arg(failCount));
}
