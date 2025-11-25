#include "stas_dialog.h"
#include <qglobal.h>
#include <qhashfunctions.h>
#include <qlocale.h>
#include <qmath.h>
#include <qobject.h>
#include <qtmetamacros.h>
ui::StasDialog::StasDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::StasDialog) {
    ui->setupUi(this);
    this->setWindowTitle("Data Statistic");
}
void ui::StasDialog::accept() { this->done(1); }
void ui::StasDialog::updateStasData(int count) {
    QString str = "共有: 个目标";
    str.insert(4, QString::number(count));
    ui->dataLabel->setText(QString(str));
}
void ui::StasDialog::startStas() {
    int colorId = ui->colorCombo->currentIndex() - 1; // -1 ALL
    int classId = ui->classCombo->currentIndex() - 1; //- 1 ALL
    int sizeId  = ui->sizeCombo->currentIndex();
    emit getStasRequested(colorId, classId, sizeId);
}
