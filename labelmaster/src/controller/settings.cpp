#include "settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QScopedPointer>
#include <QStandardPaths>

namespace controller {
QScopedPointer<QSettings> AppSettings::s_iniOverride_;

void AppSettings::initOrgApp(const QString& org, const QString& app) noexcept {
    // Best called in main() before creating QApplication/QGuiApplication.
    QCoreApplication::setOrganizationName(org);
    QCoreApplication::setApplicationName(app);
}

void AppSettings::useIniFile(const QString& iniFilePath) {
    // Ensure parent dir exists
    QFileInfo fi(iniFilePath);
    QDir().mkpath(fi.dir().absolutePath());
    s_iniOverride_.reset(new QSettings(iniFilePath, QSettings::IniFormat));
}

AppSettings& AppSettings::instance() noexcept {
    static AppSettings inst;
    return inst;
}

AppSettings::AppSettings()
    // If ini override exists, duplicate its format/path; else default org/app.
    : settings_(
          s_iniOverride_ ? QSettings(s_iniOverride_->fileName(), s_iniOverride_->format())
                         : QSettings()) {
    // Avoid reading system defaults etc. if you want only our namespace.
    settings_.setFallbacksEnabled(true); // keep default true; set false if you dislike fallbacks
}

/* ---------- dataset paths ---------- */
QString AppSettings::saveDir() const noexcept { return settings_.value(Keys::kSaveDir).toString(); }
AppSettings& AppSettings::setSaveDir(const QString& d) {
    settings_.setValue(Keys::kSaveDir, d);
    return *this;
}

QString AppSettings::lastImageDir() const noexcept {
    return settings_.value(Keys::kLastImageDir).toString();
}
AppSettings& AppSettings::setLastImageDir(const QString& d) {
    settings_.setValue(Keys::kLastImageDir, d);
    return *this;
}

/* ---------- behavior ---------- */
bool AppSettings::autoSave() const noexcept {
    return settings_.value(Keys::kAutoSave, Def::kAutoSave).toBool();
}
AppSettings& AppSettings::setAutoSave(bool v) {
    settings_.setValue(Keys::kAutoSave, v);
    return *this;
}

/* ---------- ROI ---------- */
bool AppSettings::fixedRoi() const noexcept {
    return settings_.value(Keys::kFixedRoi, Def::kFixedRoi).toBool();
}
AppSettings& AppSettings::setFixedRoi(bool v) {
    settings_.setValue(Keys::kFixedRoi, v);
    return *this;
}

int AppSettings::roiW() const noexcept { return settings_.value(Keys::kRoiW, Def::kRoiW).toInt(); }
AppSettings& AppSettings::setRoiW(int w) {
    settings_.setValue(Keys::kRoiW, w);
    return *this;
}

int AppSettings::roiH() const noexcept { return settings_.value(Keys::kRoiH, Def::kRoiH).toInt(); }
AppSettings& AppSettings::setRoiH(int h) {
    settings_.setValue(Keys::kRoiH, h);
    return *this;
}

/* ---------- sync ---------- */
void AppSettings::sync() noexcept { settings_.sync(); }
} // namespace controller