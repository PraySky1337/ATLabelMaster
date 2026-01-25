#include "settings_dialog.hpp"
#include "controller/settings.hpp"
#include "ui_settings_dialog.h"
#include "util/id_convert.hpp"
#include "ui/pixel_widgets/theme_manager.hpp"
#include <qcombobox.h>
#include <qdir.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qplaintextedit.h>
#include <qvariant.h>
#include <qwidget.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QTextStream>
using namespace ui;
using labelmaster::ui::ThemeManager;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::SettingsDialog) {
    ui_->setupUi(this);
    this->setWindowTitle("Settings");
    this->ui_->dataset_dir_edit->setText(controller::AppSettings::instance().saveDir());
    this->ui_->last_img_dir_edit->setText(controller::AppSettings::instance().lastImageDir());
    this->ui_->last_img_path_edit->setText(controller::AppSettings::instance().lastImagePath());
    this->ui_->auto_save_checkbox->setChecked(controller::AppSettings::instance().autoSave());
    this->ui_->auto_enhance_checkbox->setChecked(
        controller::AppSettings::instance().autoEnhanceV());
    this->ui_->v_rate_slider->setValue(controller::AppSettings::instance().vRate() * 10);
    this->ui_->v_rate_label->setText(
        QString::number(controller::AppSettings::instance().vRate(), 'f', 1));
    this->ui_->fix_roi_checkbox->setChecked(controller::AppSettings::instance().fixedRoi());
    this->ui_->roi_h_spin->setValue(controller::AppSettings::instance().roiH());
    this->ui_->roi_w_spin->setValue(controller::AppSettings::instance().roiW());

    // Initialize theme combo with available themes
    QStringList themes = ThemeManager::instance().availableThemes();
    ui_->theme_combo->clear();

    // Add themes with display names
    for (const QString& themeId : themes) {
        QString displayName = ThemeManager::instance().themeDisplayName(themeId);
        ui_->theme_combo->addItem(displayName, themeId);
    }

    // Set current theme
    QString currentTheme = controller::AppSettings::instance().theme();
    int themeIndex = ui_->theme_combo->findData(currentTheme);
    if (themeIndex < 0) themeIndex = 0;
    ui_->theme_combo->setCurrentIndex(themeIndex);
    update();
    connect(
        this->ui_->dataset_dir_edit, &QLineEdit::editingFinished, this,
        &SettingsDialog::setSaveDir);
    connect(
        this->ui_->last_img_dir_edit, &QLineEdit::editingFinished, this,
        &SettingsDialog::setLastImageDir);
    connect(
        this->ui_->last_img_path_edit, &QLineEdit::editingFinished, this,
        &SettingsDialog::setLastImagePath);
    connect(this->ui_->auto_save_checkbox, &QCheckBox::toggled, this, &SettingsDialog::setAutoSave);
    connect(
        this->ui_->auto_enhance_checkbox, &QCheckBox::toggled, this,
        &SettingsDialog::setAutoEnhanceV);
    connect(this->ui_->v_rate_slider, &QSlider::valueChanged, this, &SettingsDialog::setVRate);
    connect(this->ui_->fix_roi_checkbox, &QCheckBox::toggled, this, &SettingsDialog::setFixedRoi);
    connect(this->ui_->roi_h_spin, &QSpinBox::editingFinished, this, &SettingsDialog::setRoiH);
    connect(this->ui_->roi_w_spin, &QSpinBox::editingFinished, this, &SettingsDialog::setRoiW);
    connect(this->ui_->theme_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::setTheme);

    // 初始化格式选择（先设置值，再连接信号）
    ui_->format_combo->setCurrentIndex(controller::AppSettings::instance().outputFormat());
    connect(ui_->format_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        controller::AppSettings::instance().setoutputFormat(index);
    });
    connect(ui_->batch_convert_button, &QPushButton::clicked,
            this, &SettingsDialog::performBatchConvert);
}
void SettingsDialog::SaveDirEditUpdate() {
    const QString dir = QFileDialog::getExistingDirectory(
        nullptr, tr("选择图片文件夹"), QString(),
        QFileDialog::Options(QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly));
    if (!dir.isEmpty()) {
        this->ui_->dataset_dir_edit->setText(dir);
        setSaveDir();
    }
}
void SettingsDialog::LastImageDirEditUpdate() {
    const QString dir = QFileDialog::getExistingDirectory(
        nullptr, tr("选择图片文件夹"), QString(),
        QFileDialog::Options(QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly));
    if (!dir.isEmpty()) {
        this->ui_->last_img_dir_edit->setText(dir);
        setLastImageDir();
    }
}
void SettingsDialog::LastImagePathEditUpdate() {
    const QString str = QFileDialog::getOpenFileName();
    if (!str.isEmpty()) {
        this->ui_->last_img_path_edit->setText(str);
        setLastImagePath();
    }
}
void SettingsDialog::setLastImageDir() {
    controller::AppSettings::instance().setlastImageDir(this->ui_->last_img_dir_edit->text());
    update();
}
void SettingsDialog::setLastImagePath() {
    controller::AppSettings::instance().setlastImagePath(this->ui_->last_img_path_edit->text());
    update();
}
void SettingsDialog::setSaveDir() {
    controller::AppSettings::instance().setsaveDir(this->ui_->dataset_dir_edit->text());
    update();
}
void SettingsDialog::setAutoSave(bool isAutoSave) {
    controller::AppSettings::instance().setautoSave(isAutoSave);
    update();
}
void SettingsDialog::setAutoEnhanceV(bool isAutoEnhanceV) {
    controller::AppSettings::instance().setautoEnhanceV(isAutoEnhanceV);
    update();
}
void SettingsDialog::setVRate(int vRate) {
    controller::AppSettings::instance().setvRate(static_cast<float>(vRate) / 10);
    this->ui_->v_rate_label->setText(
        QString::number(controller::AppSettings::instance().vRate(), 'f', 1));
    update();
}
void SettingsDialog::setFixedRoi(bool isFixedRoi) {
    controller::AppSettings::instance().setfixedRoi(isFixedRoi);
    update();
}
void SettingsDialog::setRoiH() {
    controller::AppSettings::instance().setroiH(this->ui_->roi_h_spin->value());
    update();
}
void SettingsDialog::setRoiW() {
    controller::AppSettings::instance().setroiW(this->ui_->roi_w_spin->value());
    update();
}

// ---------- 批量替换功能 ----------
SettingsDialog::LabelInfo SettingsDialog::getLabelFromCombos(
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
        info.size = (sizeText == "Small" || sizeText == "BIg") ? 0 : 1;
    }

    // 类别: G(0), 1-5(1-5), O(6), B(7)
    QString classText = classCombo->currentText();
    info.classId = IdConvert::classToken2Id(IdConvert::normalizeClasslToken(classText));

    return info;
}

void SettingsDialog::performBatchReplace() {
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
            "请检查设置中的保存目录是否正确。");
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

        file.close();

        // 如果文件被修改，写回
        if (fileModified) {
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                out.setEncoding(QStringConverter::Utf8);
#else
                out.setCodec("UTF-8");
#endif
                for (const QString& newLine : newLines) {
                    out << newLine << '\n';
                }
                file.close();
                modifiedFiles++;
                modifiedFileNames.append(fileInfo.fileName());
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

void SettingsDialog::setTheme(int index) {
    QVariant themeData = ui_->theme_combo->itemData(index);
    if (!themeData.isValid()) {
        return; // Invalid selection
    }

    QString themeId = themeData.toString();
    controller::AppSettings::instance().settheme(themeId);

    // Apply theme immediately (hot reload)
    ThemeManager::instance().loadTheme(themeId);

    update();
}

// ---------- 批量转换功能 ----------
void SettingsDialog::performBatchConvert() {
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

        while (!in.atEnd()) {
            QString line = in.readLine();
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
                // 四个点是PnP锚点，用于透视变换SVG
                // bbox应该基于SVG的固有尺寸（viewBox）
                bool ok = true;
                auto tod = [&](int i) -> double {
                    bool o = false;
                    double v = parts[i].toDouble(&o);
                    ok &= o;
                    return v;
                };
                if (ok) {
                    // size=0:小SVG, size=1:大SVG
                    int size = parts[1].toInt();

                    // SVG固有尺寸 (viewBox)
                    // 小SVG: 557×516, 大SVG: 871×478
                    double svg_w = (size == 0) ? 557.0 : 871.0;
                    double svg_h = (size == 0) ? 516.0 : 478.0;

                    // bbox: SVG中心点(0.5, 0.5) + 归一化宽高
                    double x = 0.5;
                    double y = 0.5;
                    double w = svg_w;
                    double h = svg_h;

                    // SVG锚点 (PnP点，顺序: TL, BL, BR, TR)
                    double x0 = tod(3), y0 = tod(4);
                    double x1 = tod(5), y1 = tod(6);
                    double x2 = tod(7), y2 = tod(8);
                    double x3 = tod(9), y3 = tod(10);

                    // 输出: color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3
                    line = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15")
                        .arg(parts[0]).arg(parts[1]).arg(parts[2])
                        .arg(x).arg(y).arg(w).arg(h)
                        .arg(x0).arg(y0).arg(x1).arg(y1).arg(x2).arg(y2).arg(x3).arg(y3);
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
            else {
                // 保留原行
            }
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
