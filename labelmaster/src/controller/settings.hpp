#pragma once
#include <QScopedPointer>
#include <QSettings>
#include <QString>

namespace controller {

class AppSettings final {
public:
    // ---- lifecycle ----
    static void initOrgApp(const QString& org, const QString& app, const QString& domain) noexcept;
    static AppSettings& instance() noexcept;
    static void useIniFile(const QString& iniFilePath);

    // ---- 一次性同步 ----
    void sync() noexcept;
    // 恢复默认值
    void restoreDefaults();

    // ====== 这里用宏自动生成 getter/setter（不使用模板） ======
#define APP_SETTING_RW_STR(Name, Key, DefStr)                                                  \
    QString Name() const noexcept { return settings_.value(Key, QString(DefStr)).toString(); } \
    AppSettings& set##Name(const QString& v) {                                                 \
        settings_.setValue(Key, v);                                                            \
        return *this;                                                                          \
    }

#define APP_SETTING_RW_INT(Name, Key, DefVal)                                       \
    int Name() const noexcept { return settings_.value(Key, int(DefVal)).toInt(); } \
    AppSettings& set##Name(int v) {                                                 \
        settings_.setValue(Key, v);                                                 \
        return *this;                                                               \
    }

#define APP_SETTING_RW_FLOAT(Name, Key, DefVal)                                           \
    float Name() const noexcept { return settings_.value(Key, float(DefVal)).toFloat(); } \
    AppSettings& set##Name(float v) {                                                     \
        settings_.setValue(Key, v);                                                       \
        return *this;                                                                     \
    }

#define APP_SETING_RW_DOUBLE(Name, key, DefVal)                                              \
    double Name() const noexcept { return settings_.value(key, double(DefVal)).toDouble(); } \
    AppSettings& set##Name(double v) {                                                       \
        settings_.setValue(key, v);                                                          \
        return *this;                                                                        \
    }

#define APP_SETTING_RW_BOOL(Name, Key, DefVal)                                         \
    bool Name() const noexcept { return settings_.value(Key, bool(DefVal)).toBool(); } \
    AppSettings& set##Name(bool v) {                                                   \
        settings_.setValue(Key, v);                                                    \
        return *this;                                                                  \
    }

    // ---- 用一行声明各配置 ----
    APP_SETTING_RW_STR(saveDir, Keys::kSaveDir, Def::kSaveDir)
    APP_SETTING_RW_STR(lastImageDir, Keys::kLastImageDir, "")
    APP_SETTING_RW_STR(lastImagePath, Keys::kLastImagePath, "")
    APP_SETTING_RW_BOOL(autoSave, Keys::kAutoSave, Def::kAutoSave)
    APP_SETTING_RW_BOOL(autoEnhanceV, Keys::kAutoEnhanceV, Def::kAutoEnhanceV)
    APP_SETTING_RW_BOOL(fixedRoi, Keys::kFixedRoi, Def::kFixedRoi)
    APP_SETTING_RW_INT(roiW, Keys::kRoiW, Def::kRoiW)
    APP_SETTING_RW_INT(roiH, Keys::kRoiH, Def::kRoiH)
    APP_SETTING_RW_STR(assetsDir, Keys::kAssetsDir, Def::kAssetsDir)
    APP_SETTING_RW_FLOAT(
        numberClassifierThreshold, Keys::kNumberClassifierThreshold,
        Def::kNumberClassifierThreshold)
    APP_SETTING_RW_FLOAT(vRate, Keys::kvRate, Def::kVRate)
    APP_SETTING_RW_STR(theme, Keys::kTheme, Def::kTheme)
    APP_SETTING_RW_INT(outputFormat, Keys::kOutputFormat, Def::kOutputFormat)
    APP_SETTING_RW_INT(importFormat, Keys::kImportFormat, Def::kImportFormat)
    APP_SETTING_RW_INT(defaultClassId, Keys::kDefaultClassId, Def::kDefaultClassId)
    APP_SETTING_RW_INT(defaultColorId, Keys::kDefaultColorId, Def::kDefaultColorId)
    APP_SETTING_RW_INT(defaultSize, Keys::kDefaultSize, Def::kDefaultSize)

#undef APP_SETTING_RW_STR
#undef APP_SETTING_RW_INT
#undef APP_SETTING_RW_BOOL

    // 禁止拷贝移动
    AppSettings(const AppSettings&)            = delete;
    AppSettings& operator=(const AppSettings&) = delete;
    AppSettings(AppSettings&&)                 = delete;
    AppSettings& operator=(AppSettings&&)      = delete;

private:
    AppSettings(); // 仅 instance() 可用

    struct Keys {
        static constexpr const char* kSaveDir                   = "dataset/saveDir";
        static constexpr const char* kLastImageDir              = "dataset/lastImageDir";
        static constexpr const char* kLastImagePath             = "dataset/lastImagePath";
        static constexpr const char* kAutoSave                  = "behavior/autoSave";
        static constexpr const char* kAutoEnhanceV              = "behavior/autoEnhanceV";
        static constexpr const char* kvRate                     = "behavior/vRate";
        static constexpr const char* kFixedRoi                  = "roi/fixed";
        static constexpr const char* kRoiW                      = "roi/w";
        static constexpr const char* kRoiH                      = "roi/h";
        static constexpr const char* kAssetsDir                 = "assets/directory";
        static constexpr const char* kNumberClassifierThreshold = "detector/tradition/threshold";
        static constexpr const char* kTheme                     = "ui/theme";
        static constexpr const char* kOutputFormat              = "dataset/outputFormat";
        static constexpr const char* kImportFormat              = "dataset/importFormat";
        static constexpr const char* kDefaultClassId            = "annotation/defaultClassId";
        static constexpr const char* kDefaultColorId            = "annotation/defaultColorId";
        static constexpr const char* kDefaultSize               = "annotation/defaultSize";
    };
    struct Def {
        static constexpr const char* kSaveDir             = "label";
        static constexpr const char* kAssetsDir           = "/usr/opt/LabelMaster/assets"; //应用启动时会自动设置当前Application目录
        static constexpr bool kAutoSave                   = false;
        static constexpr bool kAutoEnhanceV               = false;
        static constexpr bool kFixedRoi                   = false;
        static constexpr int kRoiW                        = 640;
        static constexpr int kRoiH                        = 480;
        static constexpr float kNumberClassifierThreshold = 80.f;
        static constexpr const float kVRate               = 4;
        static constexpr const char* kTheme               = "retro";
        static constexpr const int kOutputFormat          = 0;  // 0=pts-only(11), 1=xywh+pts(15)
        static constexpr const int kImportFormat          = 0;  // 0=LabelMasterV2, 1=LabelMasterV1, 2=LabelMasterV3, 3=HITSZ, 4=UPC, 5=NMPU
        static constexpr const int kDefaultClassId        = 0;  // G
        static constexpr const int kDefaultColorId        = 2;  // Gray
        static constexpr const int kDefaultSize           = 0;  // Small
    };

    QSettings settings_;
    static QScopedPointer<QSettings> s_iniOverride_;
};

} // namespace controller
