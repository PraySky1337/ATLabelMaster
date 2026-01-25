/**
 * @file crash_handler.hpp
 * @brief Crash handler and recovery system for ATLabelMaster
 *
 * Provides automatic crash detection, logging, and recovery
 * of unsaved work.
 */

#ifndef LABELMASTER_CRASH_HANDLER_HPP
#define LABELMASTER_CRASH_HANDLER_HPP

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariantMap>

namespace labelmaster::util {

/**
 * @brief Crash recovery data
 */
struct CrashRecoveryData {
    QString lastImagePath;
    QString lastProjectPath;
    QByteArray unsavedAnnotations;
    qint64 lastSaveTime = 0;
    int currentClassIndex = 0;
    QVariantMap customData;
};

/**
 * @brief Crash handler singleton
 *
 * Handles application crashes by:
 * - Detecting abnormal termination
 * - Saving crash dumps
 * - Logging crash information
 * - Offering recovery of unsaved work
 */
class CrashHandler : public QObject {
    Q_OBJECT

public:
    static CrashHandler& instance();

    /**
     * @brief Initialize the crash handler
     * @param appName Application name
     */
    void initialize(const QString& appName = "ATLabelMaster");

    /**
     * @brief Save current state for recovery
     */
    void saveState(const CrashRecoveryData& data);

    /**
     * @brief Load saved state after crash
     */
    CrashRecoveryData loadState();

    /**
     * @brief Check if there's a recoverable state from a crash
     */
    bool hasRecoverableState() const;

    /**
     * @brief Clear saved state (call after successful recovery or manual clear)
     */
    void clearState();

    /**
     * @brief Get crash log file path
     */
    QString crashLogPath() const;

    /**
     * @brief Write to crash log
     */
    void logCrash(const QString& message);

signals:
    /**
     * @brief Emitted when a crash is detected on startup
     */
    void crashDetected(const QString& message);

    /**
     * @brief Emitted when state is saved
     */
    void stateSaved();

private:
    CrashHandler();
    ~CrashHandler() override = default;

    // Delete copy/move
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

    void setupSignalHandlers();
    QString stateFilePath() const;
    QString lockFilePath() const;

private:
    QString app_name_;
    bool initialized_ = false;
};

/**
 * @brief RAII class for auto-saving state
 *
 * Automatically saves state on destruction.
 */
class AutoSaveGuard {
public:
    AutoSaveGuard();
    ~AutoSaveGuard();

    void setData(const CrashRecoveryData& data);
    CrashRecoveryData data() const;

private:
    CrashRecoveryData data_;
};

} // namespace labelmaster::util

#endif // LABELMASTER_CRASH_HANDLER_HPP
