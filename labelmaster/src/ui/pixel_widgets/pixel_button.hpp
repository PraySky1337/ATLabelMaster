/**
 * @file pixel_button.hpp
 * @brief Pixel art style button widget for ATLabelMaster
 *
 * Custom button widget with pixel art styling support.
 * Features:
 * - Sharp corners (no rounded edges)
 * - Distinct border
 * - Pixel-perfect hover/pressed states
 * - Theme-aware colors
 */

#ifndef LABELMASTER_PIXEL_BUTTON_HPP
#define LABELMASTER_PIXEL_BUTTON_HPP

#include <QPushButton>
#include <QToolButton>
#include <QPaintEvent>
#include <QEvent>

namespace labelmaster::ui {

/**
 * @brief Pixel art style button
 *
 * A QPushButton subclass that renders in pixel art style.
 * Automatically uses colors from the current theme.
 */
class PixelButton : public QPushButton {
    Q_OBJECT

public:
    explicit PixelButton(QWidget* parent = nullptr);
    explicit PixelButton(const QString& text, QWidget* parent = nullptr);
    explicit PixelButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

    void setPixelBorderWidth(int width);
    int pixelBorderWidth() const;

    void setShowPressedEffect(bool show);
    bool showPressedEffect() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event) override;

private:
    void initStyle();
    QColor getButtonColor() const;
    QColor getButtonHoverColor() const;
    QColor getButtonPressedColor() const;
    QColor getBorderColor() const;
    QColor getTextColor() const;

private:
    int border_width_ = 2;
    bool show_pressed_effect_ = true;
    bool is_hovered_ = false;
};

/**
 * @brief Pixel art tool button (for toolbar)
 */
class PixelToolButton : public QToolButton {
    Q_OBJECT

public:
    explicit PixelToolButton(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_PIXEL_BUTTON_HPP
