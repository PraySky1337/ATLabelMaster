#pragma once
#include <QSettings>
#include <QString>

namespace controller {
/// Lightweight typed wrapper around QSettings.
/// Usage:
///   AppSettings::initOrgApp("ATLabelMaster","ATLabelMaster");
///   auto& s = AppSettings::instance();
///   s.setSaveDir("/data").setAutoSave(true).sync();
///   QString dir = s.saveDir();
class AppSettings final {
public:
    // ---- lifecycle ----
    static void initOrgApp(const QString& org, const QString& app) noexcept;
    static AppSettings& instance() noexcept;

    // Optional: switch to explicit INI file (e.g., ~/.config/ATLabelMaster/settings.ini)
    // Call once at startup if you prefer file-based settings over platform registry.
    static void useIniFile(const QString& iniFilePath);

    // ---- dataset paths ----
    [[nodiscard]] QString saveDir()      const noexcept;
    AppSettings&          setSaveDir(const QString& d);

    [[nodiscard]] QString lastImageDir() const noexcept;
    AppSettings&          setLastImageDir(const QString& d);

    // ---- behavior ----
    [[nodiscard]] bool autoSave()  const noexcept;
    AppSettings&        setAutoSave(bool v);

    // ---- ROI ----
    [[nodiscard]] bool fixedRoi()  const noexcept;
    AppSettings&        setFixedRoi(bool v);

    [[nodiscard]] int  roiW()      const noexcept;
    AppSettings&        setRoiW(int w);

    [[nodiscard]] int  roiH()      const noexcept;
    AppSettings&        setRoiH(int h);

    // ---- misc ----
    // Force flush to storage immediately. Usually QSettings auto-syncs, but
    // calling this after a batch of updates is safer for crash resilience.
    void sync() noexcept;

    // Non-copyable / Non-movable single instance
    AppSettings(const AppSettings&)            = delete;
    AppSettings& operator=(const AppSettings&) = delete;
    AppSettings(AppSettings&&)                 = delete;
    AppSettings& operator=(AppSettings&&)      = delete;

private:
    AppSettings(); // private: use instance()

    // Centralized keys & defaults
    struct Keys {
        // groups/keys (flat keys for simplicity)
        static constexpr const char* kSaveDir       = "dataset/saveDir";
        static constexpr const char* kLastImageDir  = "dataset/lastImageDir";
        static constexpr const char* kAutoSave      = "behavior/autoSave";
        static constexpr const char* kFixedRoi      = "roi/fixed";
        static constexpr const char* kRoiW          = "roi/w";
        static constexpr const char* kRoiH          = "roi/h";
    };
    struct Def {
        static constexpr bool kAutoSave   = false;
        static constexpr bool kFixedRoi   = false;
        static constexpr int  kRoiW       = 640;
        static constexpr int  kRoiH       = 640;
    };

    // Underlying storage
    QSettings settings_;
    // If set via useIniFile(), we store a separate instance using that file.
    // Otherwise, we use the org/app ctor.
    static QScopedPointer<QSettings> s_iniOverride_;
};
}