#include "info_dialog.h"
#include "ui_info_dialog.h"
#include <qcombobox.h>
#include <qdialog.h>
#include <qglobal.h>
#include <qguiapplication_platform.h>
#include <qobject.h>
#include <qpoint.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtransform.h>
#include <qwidget.h>
using namespace ui;
InfoDialog::InfoDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InfoDialog) {
    ui->setupUi(this);
    this->setWindowTitle("Edit Info");
};

InfoDialog::~InfoDialog() { delete this->ui; }
// 取消
void InfoDialog::reject() { this->done(1); }
// 确定
void InfoDialog::accept() {
    QString text = ui->colorCombo->currentText();
    if (text == "Red") {
        text = "R";
    } else if (text == "Blue") {
        text = "B";
    } else if (text == "Purple") {
        text = "P";
    } else {
        text = "G";
    }
    emit InfoGetted(
        this->ui->classCombo->currentText(), text, ui->sizeCombo->currentIndex(), _isCurrent);
    this->done(1);
}
void InfoDialog::updateInfo(
    bool isCurrent, const QString& defaultClass, const QString& defaultColor,
    const int& defaultSize) {
    QString Color;
    if (defaultColor == "R") {
        Color = "Red";
    } else if (defaultColor == "B") {
        Color = "Blue";
    } else if (defaultColor == "P") {
        Color = "Purple";
    } else {
        Color = "Gray";
    }
    _isCurrent = isCurrent;
    ui->colorCombo->setCurrentText(Color);
    ui->sizeCombo->setCurrentIndex(defaultSize);
    ui->classCombo->setCurrentText(defaultClass);
    connect(
        ui->classCombo, &QComboBox::currentIndexChanged, this,
        &InfoDialog::updateSize); // 接收到初始数据后再连接
}
void InfoDialog::updateSize(int index) {
    if (index != 1) {
        ui->sizeCombo->setCurrentIndex(0);
    } else {
        ui->sizeCombo->setCurrentIndex(1);
    }
}
