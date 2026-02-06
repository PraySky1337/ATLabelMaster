#include "settings_dialog.hpp"
#include "controller/settings.hpp"
#include "ui_settings_dialog.h"
#include "util/id_convert.hpp"
#include "util/svg_constants.hpp"
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
#include <QTransform>
#include <QPolygonF>
#include <limits>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QTextStream>
using namespace ui;
using labelmaster::ui::ThemeManager;

SettingsDialog::SettingsDialog(QWidget* parent)
    : PixelDialog(parent)
    , ui_(new Ui::SettingsDialog) {
    ui_->setupUi(this);
    this->setWindowTitle("Settings");
    this->ui_->dataset_dir_edit->setText(controller::AppSettings::instance().saveDir());
    this->ui_->last_img_dir_edit->setText(controller::AppSettings::instance().lastImageDir());
    this->ui_->last_img_path_edit->setText(controller::AppSettings::instance().lastImagePath());
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
    connect(this->ui_->fix_roi_checkbox, &QCheckBox::toggled, this, &SettingsDialog::setFixedRoi);
    connect(this->ui_->roi_h_spin, &QSpinBox::editingFinished, this, &SettingsDialog::setRoiH);
    connect(this->ui_->roi_w_spin, &QSpinBox::editingFinished, this, &SettingsDialog::setRoiW);
    connect(this->ui_->theme_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::setTheme);
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
