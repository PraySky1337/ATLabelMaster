/**
 * @file crash_handler.cpp
 * @brief Implementation of crash handler
 */

#include "crash_handler.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <csignal>
#include <exception>
#include <iostream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <execinfo.h>
#endif

namespace labelmaster::util {

namespace {
    CrashHandler* g_instance = nullptr;

    // Signal handler for Unix systems
    void signalHandler(int signal) {
        if (g_instance) {
            QString sigName;
            switch (signal) {
                case SIGSEGV: sigName = "SIGSEGV"; break;
                case SIGABRT: sigName = "SIGABRT"; break;
                case SIGFPE:  sigName = "SIGFPE"; break;
                case SIGILL:  sigName = "SIGILL"; break;
                default:      sigName = QString::number(signal); break;
            }

            QString message = QString("Fatal signal: %1").arg(sigName);
            g_instance->logCrash(message);

#ifdef Q_OS_UNIX
            // Print stack trace
            void* array[32];
            size_t size = backtrace(array, 32);
            char** strings = backtrace_symbols(array, size);

            QFile logFile(g_instance->crashLogPath());
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream stream(&logFile);
                stream << "\n=== Stack Trace ===\n";
                for (size_t i = 0; i < size; i++) {
                    stream << strings[i] << "\n";
                }
                stream << "===================\n";
            }
            free(strings);
#endif
        }

        // Execute default signal handler
        std::signal(signal, SIG_DFL);
        std::raise(signal);
    }

    // Termination handler
    void terminateHandler() {
        static bool tried_throw = false;
        try {
            // Try to throw current exception
            if (!tried_throw) {
                tried_throw = true;
                throw;
            }
        } catch (const std::exception& e) {
            if (g_instance) {
                g_instance->logCrash(QString("Unhandled exception: %1").arg(e.what()));
            }
        } catch (...) {
            if (g_instance) {
                g_instance->logCrash("Unhandled exception of unknown type");
            }
        }

        // Abort to trigger signal handler
        std::abort();
    }
}

CrashHandler& CrashHandler::instance() {
    if (!g_instance) {
        g_instance = new CrashHandler();
    }
    return *g_instance;
}

CrashHandler::CrashHandler() {
    // Check for previous crash
    QString lockFile = lockFilePath();
    if (QFile::exists(lockFile)) {
        emit crashDetected("Previous session terminated abnormally");
    }
}

void CrashHandler::initialize(const QString& appName) {
    app_name_ = appName;
    initialized_ = true;

    // Create crash handler directory
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    dir.mkpath("crash_handler");

    // Set up signal handlers
    setupSignalHandlers();

    // Create lock file
    QFile lock(lockFilePath());
    if (lock.open(QIODevice::WriteOnly)) {
        lock.write(QDateTime::currentDateTime().toString().toUtf8());
        lock.close();
    }
}

void CrashHandler::setupSignalHandlers() {
#ifdef Q_OS_UNIX
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGILL, signalHandler);
#endif

    std::set_terminate(terminateHandler);
}

QString CrashHandler::stateFilePath() const {
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    return dir.filePath("crash_handler/recovery_state.json");
}

QString CrashHandler::lockFilePath() const {
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    return dir.filePath("crash_handler/session.lock");
}

QString CrashHandler::crashLogPath() const {
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return dir.filePath(QString("crash_handler/crash_%1.log").arg(timestamp));
}

void CrashHandler::saveState(const CrashRecoveryData& data) {
    QJsonObject json;
    json["last_image_path"] = data.lastImagePath;
    json["last_project_path"] = data.lastProjectPath;
    json["unsaved_annotations"] = QString(data.unsavedAnnotations.toBase64());
    json["last_save_time"] = data.lastSaveTime;
    json["current_class_index"] = data.currentClassIndex;

    QJsonDocument doc(json);

    QFile file(stateFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        emit stateSaved();
    }
}

CrashRecoveryData CrashHandler::loadState() {
    CrashRecoveryData data;

    QFile file(stateFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isNull() && doc.isObject()) {
            QJsonObject json = doc.object();
            data.lastImagePath = json["last_image_path"].toString();
            data.lastProjectPath = json["last_project_path"].toString();
            data.unsavedAnnotations = QByteArray::fromBase64(
                json["unsaved_annotations"].toString().toLocal8Bit());
            data.lastSaveTime = json["last_save_time"].toVariant().toLongLong();
            data.currentClassIndex = json["current_class_index"].toInt();
        }
    }

    return data;
}

bool CrashHandler::hasRecoverableState() const {
    return QFile::exists(stateFilePath());
}

void CrashHandler::clearState() {
    QFile::remove(stateFilePath());
    QFile::remove(lockFilePath());
}

void CrashHandler::logCrash(const QString& message) {
    QFile logFile(crashLogPath());
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
               << " - " << message << "\n";
    }
}

//=============================================================================
// AutoSaveGuard
//=============================================================================

AutoSaveGuard::AutoSaveGuard() {
    // Load existing state on construction
    data_ = CrashHandler::instance().loadState();
}

AutoSaveGuard::~AutoSaveGuard() {
    // Save state on destruction
    if (!data_.lastImagePath.isEmpty() || !data_.unsavedAnnotations.isEmpty()) {
        CrashHandler::instance().saveState(data_);
    }

    // Clear lock file to indicate normal exit
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    QFile::remove(dir.filePath("crash_handler/session.lock"));
}

void AutoSaveGuard::setData(const CrashRecoveryData& data) {
    data_ = data;
}

CrashRecoveryData AutoSaveGuard::data() const {
    return data_;
}

} // namespace labelmaster::util
