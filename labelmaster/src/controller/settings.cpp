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

} // namespace controller