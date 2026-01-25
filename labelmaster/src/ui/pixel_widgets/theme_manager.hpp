/**
 * @file theme_manager.hpp
 * @brief Theme Manager for pixel art style themes in ATLabelMaster
 *
 * Manages loading, switching, and accessing pixel art themes.
 * Supports multiple themes (Retro Gaming, Dark Modern, Classic).
 */

#ifndef LABELMASTER_THEME_MANAGER_HPP
#define LABELMASTER_THEME_MANAGER_HPP

#include <QObject>
#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QFont>
#include <QHash>
#include <QApplication>

namespace labelmaster::ui {

/**
 * @brief Theme Manager class for pixel art styling
 *
 * Singleton class that manages theme loading, switching, and provides
 * access to theme colors, fonts, and styling properties.
 *
 * Usage:
 *   ThemeManager::instance().loadTheme("retro");
 *   QColor bgColor = ThemeManager::instance().color("background");
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     * @return Reference to ThemeManager instance
     */
    static ThemeManager& instance();

    /**
     * @brief Load a theme by ID
     * @param themeId Theme identifier (e.g., "retro", "dark", "classic")
     * @return true if theme loaded successfully
     */
    bool loadTheme(const QString& themeId);

    /**
     * @brief Get current theme ID
     * @return Current theme identifier
     */
    QString currentThemeId() const;

    /**
     * @brief Get list of available themes
     * @return List of available theme IDs
     */
    QStringList availableThemes() const;

    /**
     * @brief Get theme display name
     * @param themeId Theme identifier
     * @return Human-readable theme name
     */
    QString themeDisplayName(const QString& themeId) const;

    /**
     * @brief Get a color from the current theme
     * @param key Color key (e.g., "background", "accent_primary")
     * @return QColor value, or black if not found
     */
    QColor color(const QString& key) const;

    /**
     * @brief Get a color with fallback
     * @param key Color key
     * @param fallback Fallback color if key not found
     * @return QColor value
     */
    QColor color(const QString& key, const QColor& fallback) const;

    /**
     * @brief Get UI font for current theme
     * @return QFont for UI elements
     */
    QFont uiFont() const;

    /**
     * @brief Get monospace font for current theme
     * @return QFont for monospace elements
     */
    QFont monoFont() const;

    /**
     * @brief Get dimension value from theme
     * @param key Dimension key (e.g., "button_border", "panel_border")
     * @return Integer value, or 0 if not found
     */
    int dimension(const QString& key) const;

    /**
     * @brief Get dimension with fallback
     * @param key Dimension key
     * @param fallback Fallback value
     * @return Integer value
     */
    int dimension(const QString& key, int fallback) const;

    /**
     * @brief Generate Qt StyleSheet for current theme
     * @return Complete QSS string for application
     */
    QString generateStyleSheet() const;

    /**
     * @brief Apply current theme to QApplication
     */
    void applyTheme();

    /**
     * @brief Get theme assets directory path
     * @param themeId Theme identifier
     * @return Path to theme assets directory
     */
    QString themeAssetsPath(const QString& themeId) const;

    /**
     * @brief Check if pixel scaling is enabled for current theme
     * @return true if pixel scaling should be applied
     */
    bool pixelScaling() const;

    /**
     * @brief Get animation speed setting
     * @return Animation speed (e.g., "fast", "medium", "slow", "none")
     */
    QString animationSpeed() const;

signals:
    /**
     * @brief Emitted when theme changes
     * @param themeId New theme ID
     */
    void themeChanged(const QString& themeId);

private:
    ThemeManager();
    ~ThemeManager() override = default;

    // Delete copy/move constructors
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    ThemeManager(ThemeManager&&) = delete;
    ThemeManager& operator=(ThemeManager&&) = delete;

    /**
     * @brief Discover available themes in assets directory
     */
    void discoverThemes();

    /**
     * @brief Load theme JSON file
     * @param themeId Theme identifier
     * @return QJsonObject of theme data
     */
    QJsonObject loadThemeJson(const QString& themeId) const;

    /**
     * @brief Get color from JSON object
     * @param obj JSON object containing color data
     * @param key Color key
     * @return QColor value
     */
    QColor parseColor(const QJsonObject& obj, const QString& key) const;

    /**
     * @brief Generate stylesheet for specific widget type
     * @param widgetType Widget type (e.g., "QPushButton", "QDialog")
     * @return QSS string for widget type
     */
    QString widgetStyleSheet(const QString& widgetType) const;

private:
    QJsonObject m_currentTheme;
    QString m_currentThemeId;
    QHash<QString, QJsonObject> m_availableThemes;
    QString m_assetsPath;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_THEME_MANAGER_HPP
