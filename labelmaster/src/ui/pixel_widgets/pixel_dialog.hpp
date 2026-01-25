/**
 * @file pixel_dialog.hpp
 * @brief Pixel art style dialog base class
 */

#ifndef LABELMASTER_PIXEL_DIALOG_HPP
#define LABELMASTER_PIXEL_DIALOG_HPP

#include <QDialog>
#include <QPaintEvent>

namespace labelmaster::ui {

/**
 * @brief Base class for pixel art styled dialogs
 *
 * Automatically applies pixel theme styling.
 * All dialogs should inherit from this for consistent pixel styling.
 */
class PixelDialog : public QDialog {
    Q_OBJECT

public:
    explicit PixelDialog(QWidget* parent = nullptr);
    explicit PixelDialog(const QString& title, QWidget* parent = nullptr);

    void setDialogBorderWidth(int width);
    int dialogBorderWidth() const;

    void setApplyThemeOnShow(bool apply);
    bool applyThemeOnShow() const;

protected:
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void initDialog();

private:
    int border_width_ = 3;
    bool apply_theme_on_show_ = true;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_PIXEL_DIALOG_HPP
