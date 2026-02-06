#include "ui_dataset_dialog.h"
#include "pixel_widgets/pixel_dialog.hpp"
#include <QComboBox>
namespace ui {
class DatasetDialog : public labelmaster::ui::PixelDialog {
    Q_OBJECT
public:
    explicit DatasetDialog(QWidget* parent = nullptr);
    ~DatasetDialog() override;

private:
    Ui::DatasetDialog* ui_;

    struct LabelInfo {
        int colorId;
        int size;
        int classId;
    };

    LabelInfo getLabelFromCombos(
        QComboBox* colorCombo, QComboBox* sizeCombo,
        QComboBox* classCombo) const;

private slots:
    void performBatchReplace();
    void performBatchConvert();
    void setimportFormat(int index);
};

} // namespace ui
