#pragma once
#include <QCoreApplication>
#include <QDateTime>
#include <QMetaObject>
#include <QMutex>
#include <QObject>
#include <QTextEdit>
#include <cstdio>

namespace logger {

class Logger : public QObject {
    Q_OBJECT
public:
    enum class Level { Info, Warn, Error, Debug };

    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void attachTextEdit(QTextEdit* edit) {
        QMutexLocker lock(&mutex_);
        logView_ = edit;
    }

    // 普通日志（非 Qt handler 场景）
    void log(Level lvl, const QString& msg) { writeLine(lvl, msg, /*fromQtHandler=*/false); }
    inline void info(const QString& s) { log(Level::Info, s); }
    inline void warn(const QString& s) { log(Level::Warn, s); }
    inline void error(const QString& s) { log(Level::Error, s); }
    inline void debug(const QString& s) { log(Level::Debug, s); }

    // 安装 Qt 全局消息处理器（无递归、线程安全）
    static void installQtHandler() {
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& msg) {
            auto& L   = Logger::instance();
            Level lvl = Level::Info;
            switch (type) {
            case QtDebugMsg: lvl = Level::Debug; break;
            case QtWarningMsg: lvl = Level::Warn; break;
            case QtCriticalMsg: lvl = Level::Error; break;
            case QtFatalMsg: lvl = Level::Error; break;
            default: lvl = Level::Info; break;
            }
            L.writeLine(lvl, msg, /*fromQtHandler=*/true);
            if (type == QtFatalMsg)
                std::abort();
        });
    }

private:
    explicit Logger(QObject* parent = nullptr)
        : QObject(parent) {}

    void writeLine(Level lvl, const QString& msg, bool fromQtHandler) {
        static const QHash<Level, QString> tag{
            { Level::Info,  "[INFO]"},
            { Level::Warn,  "[WARN]"},
            {Level::Error, "[ERROR]"},
            {Level::Debug, "[DEBUG]"},
        };
        static const QHash<Level, const char*> color{
            { Level::Info, "\033[32m"}, // 绿色
            { Level::Warn, "\033[33m"}, // 黄色
            {Level::Error, "\033[31m"}, // 红色
            {Level::Debug, "\033[36m"}, // 青色
        };
        static constexpr const char* reset = "\033[0m";

        const QString line = QString("%1 %2 %3")
                                 .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                                 .arg(tag.value(lvl))
                                 .arg(msg);

        // UI 日志
        {
            QMutexLocker lock(&mutex_);
            if (logView_) {
                auto* edit = logView_;
                QMetaObject::invokeMethod(
                    edit, [edit, line] { edit->append(line); }, Qt::QueuedConnection);
            }
        }

        // 控制台彩色输出
        QByteArray utf8 = line.toUtf8();
        std::fprintf(stderr, "%s%s%s\n", color.value(lvl), utf8.constData(), reset);
        std::fflush(stderr);
    }

    QTextEdit* logView_ = nullptr;
    QMutex mutex_;
};

} // namespace logger

// 宏
#define LOGI(...) logger::Logger::instance().info(__VA_ARGS__)
#define LOGW(...) logger::Logger::instance().warn(__VA_ARGS__)
#define LOGE(...) logger::Logger::instance().error(__VA_ARGS__)
#define LOGD(...) logger::Logger::instance().debug(__VA_ARGS__)
