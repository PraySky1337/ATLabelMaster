/**
 * @file pixel_checkbox.hpp
 * @brief Pixel art style checkbox and radio button widgets
 */

#ifndef LABELMASTER_PIXEL_CHECKBOX_HPP
#define LABELMASTER_PIXEL_CHECKBOX_HPP

#include <QCheckBox>
#include <QRadioButton>
#include <QPaintEvent>

namespace labelmaster::ui {

/**
 * @brief Pixel art style checkbox
 *
 * Square checkbox with pixel art styling.
 * No rounded corners, block-style checkmark.
 */
class PixelCheckBox : public QCheckBox {
    Q_OBJECT

public:
    explicit PixelCheckBox(QWidget* parent = nullptr);
    explicit PixelCheckBox(const QString& text, QWidget* parent = nullptr);

    void setBoxSize(int size);
    int boxSize() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void initStyle();

    QRect boxRect() const;
    void drawCheckMark(QPainter& painter, const QRect& rect) const;

private:
    int box_size_ = 18;
};

/**
 * @brief Pixel art style radio button
 *
 * Square radio button (pixel style - no circles!)
 */
class PixelRadioButton : public QRadioButton {
    Q_OBJECT

public:
    explicit PixelRadioButton(QWidget* parent = nullptr);
    explicit PixelRadioButton(const QString& text, QWidget* parent = nullptr);

    void setBoxSize(int size);
    int boxSize() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void initStyle();

    QRect boxRect() const;
    void drawCheckedIndicator(QPainter& painter, const QRect& rect) const;

private:
    int box_size_ = 18;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_PIXEL_CHECKBOX_HPP
