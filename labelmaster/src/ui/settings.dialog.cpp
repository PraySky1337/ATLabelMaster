#include "settings_dialog.hpp"
#include "ui_settings_dialog.h"
#include "app_settings.hpp"
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    loadFromSettings();
    connect(ui->choose_dataset_dir_button, &QPushButton::clicked,
            this, &SettingsDialog::onChooseSaveDir);
    connect(ui->choose_last_img_dir_button, &QPushButton::clicked,
            this, &SettingsDialog::onChooseLastImgDir);
    connect(ui->buttonBox_ok,     &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(ui->buttonBox_cancel, &QPushButton::clicked, this, &SettingsDialog::onReject);
}

SettingsDialog::~SettingsDialog() { delete ui; }

void SettingsDialog::loadFromSettings() {
    auto& s = AppSettings::instance();
    ui->dataset_dir_edit->setText(s.saveDir());
    ui->last_img_dir_edit->setText(s.lastImageDir());
    ui->auto_save_checkbox->setChecked(s.autoSave());
    ui->fix_roi_checkbox->setChecked(s.fixedRoi());
    ui->roi_w_spin->setValue(s.roiW());
    ui->roi_h_spin->setValue(s.roiH());
}

void SettingsDialog::saveToSettings() {
    auto& s = AppSettings::instance();
    s.setSaveDir(ui->dataset_dir_edit->text());
    s.setLastImageDir(ui->last_img_dir_edit->text());
    s.setAutoSave(ui->auto_save_checkbox->isChecked());
    s.setFixedRoi(ui->fix_roi_checkbox->isChecked());
    s.setRoiW(ui->roi_w_spin->value());
    s.setRoiH(ui->roi_h_spin->value());
}

void SettingsDialog::onChooseSaveDir() {
    QString d = QFileDialog::getExistingDirectory(this, "选择数据集保存目录");
    if (!d.isEmpty()) ui->dataset_dir_edit->setText(d);
}
void SettingsDialog::onChooseLastImgDir() {
    QString d = QFileDialog::getExistingDirectory(this, "选择图片根目录");
    if (!d.isEmpty()) ui->last_img_dir_edit->setText(d);
}
void SettingsDialog::onAccept() { saveToSettings(); accept(); }
void SettingsDialog::onReject() { reject(); }
