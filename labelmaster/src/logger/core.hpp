#pragma once
#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QTextEdit>
#include <QMetaObject>
#include <QCoreApplication>
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
    void log(Level lvl, const QString& msg) {
        writeLine(lvl, msg, /*fromQtHandler=*/false);
    }
    inline void info (const QString& s){ log(Level::Info,  s); }
    inline void warn (const QString& s){ log(Level::Warn,  s); }
    inline void error(const QString& s){ log(Level::Error, s); }
    inline void debug(const QString& s){ log(Level::Debug, s); }

    // 安装 Qt 全局消息处理器（无递归、线程安全）
    static void installQtHandler() {
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& msg) {
            auto& L = Logger::instance();
            Level lvl = Level::Info;
            switch (type) {
                case QtDebugMsg:    lvl = Level::Debug; break;
                case QtWarningMsg:  lvl = Level::Warn;  break;
                case QtCriticalMsg: lvl = Level::Error; break;
                case QtFatalMsg:    lvl = Level::Error; break;
                default:            lvl = Level::Info;  break;
            }
            L.writeLine(lvl, msg, /*fromQtHandler=*/true);
            if (type == QtFatalMsg) std::abort();
        });
    }

private:
    explicit Logger(QObject* parent=nullptr): QObject(parent) {}

    void writeLine(Level lvl, const QString& msg, bool fromQtHandler) {
        static const QHash<Level, QString> tag{
            {Level::Info,  "[INFO]"},
            {Level::Warn,  "[WARN]"},
            {Level::Error, "[ERROR]"},
            {Level::Debug, "[DEBUG]"},
        };
        const QString line = QString("%1 %2 %3")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
            .arg(tag.value(lvl))
            .arg(msg);

        // 1) UI 线程安全更新
        {
            QMutexLocker lock(&mutex_);
            if (logView_) {
                auto* edit = logView_;
                QMetaObject::invokeMethod(edit, [edit, line]{
                    edit->append(line);
                }, Qt::QueuedConnection);
            }
        }

        // 2) 控制台输出：handler 内禁止再走 Qt 日志，直接写 stderr
        if (fromQtHandler) {
            std::fputs((line + '\n').toUtf8().constData(), stderr);
            std::fflush(stderr);
        } else {
            // 普通日志可以直接写 stderr；避免再次触发 handler
            std::fputs((line + '\n').toUtf8().constData(), stderr);
            std::fflush(stderr);
        }
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
