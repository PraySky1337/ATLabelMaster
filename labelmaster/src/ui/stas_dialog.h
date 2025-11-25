#include "centered_dialog.h"
#include "qobject.h"
#include "ui_stas_dialog.h"
#include <qdialog.h>
#include <qlistview.h>
#include <qobjectdefs.h>
#include <qtmetamacros.h>
#include <qwidget.h>
namespace ui {
class StasDialog : public QDialog {
    Q_OBJECT
public:
    explicit StasDialog(QWidget* parent = nullptr);

signals:
    void getStasRequested(int colorId = -1, int classId = -1, int sizeId = 0);

private:
    Ui::StasDialog* ui;
private slots:
    void accept() override;

public slots:
    void updateStasData(int const);
    void startStas();
};
} // namespace ui
