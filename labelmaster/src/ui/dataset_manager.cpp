#include "ui/dataset_manager.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

static const char* kAppDir = ".atlabelmaster";
static const char* kCfg    = "config.json";

DatasetManager& DatasetManager::instance() {
    static DatasetManager inst;
    return inst;
}

DatasetManager::DatasetManager() {
    // 读取已有配置
    QFile f(cfgPath());
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (doc.isObject()) {
            auto o = doc.object();
            saveDir_  = o.value("save_dir").toString();
            imageDir_ = o.value("image_dir").toString();
        }
    }
}

QString DatasetManager::cfgPath() const {
    QString home = QDir::homePath();
    QDir d(home + "/" + kAppDir);
    if (!d.exists()) d.mkpath(".");
    return d.filePath(kCfg);
}

void DatasetManager::setSaveDir(const QString& path) {
    saveDir_ = path;
    // 写回配置（不改 progress）
    QFile f(cfgPath());
    QJsonObject o;
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (doc.isObject()) o = doc.object();
    }
    o.insert("save_dir", saveDir_);
    if (!imageDir_.isEmpty()) o.insert("image_dir", imageDir_);
    QFile f2(cfgPath());
    if (f2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f2.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f2.close();
    }
}

QString DatasetManager::saveDir() const {
    return saveDir_;
}

void DatasetManager::setImageDir(const QString& imageDir) {
    imageDir_ = imageDir;
    QFile f(cfgPath());
    QJsonObject o;
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (doc.isObject()) o = doc.object();
    }
    o.insert("image_dir", imageDir_);
    QFile f2(cfgPath());
    if (f2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f2.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f2.close();
    }
}

QString DatasetManager::imageDir() const {
    return imageDir_;
}

void DatasetManager::saveProgress(int currentIndex) {
    if (saveDir_.isEmpty() || imageDir_.isEmpty()) return;

    // 写进 saveDir/progress.json（按 imageDir 记 key）
    QDir d(saveDir_);
    if (!d.exists()) d.mkpath(".");
    QString p = d.filePath("progress.json");

    QJsonObject o;
    QFile f(p);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (doc.isObject()) o = doc.object();
    }
    o.insert(imageDir_, currentIndex);

    QFile f2(p);
    if (f2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f2.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f2.close();
    }

    // 同步到全局 config.json（冗余：方便下次直接恢复环境）
    QFile fc(cfgPath());
    QJsonObject oc;
    if (fc.exists() && fc.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(fc.readAll());
        fc.close();
        if (doc.isObject()) oc = doc.object();
    }
    oc.insert("save_dir", saveDir_);
    oc.insert("image_dir", imageDir_);
    QFile fc2(cfgPath());
    if (fc2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fc2.write(QJsonDocument(oc).toJson(QJsonDocument::Indented));
        fc2.close();
    }
}

int DatasetManager::loadProgress() const {
    if (saveDir_.isEmpty() || imageDir_.isEmpty()) return -1;
    QDir d(saveDir_);
    QString p = d.filePath("progress.json");
    QFile f(p);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return -1;
    auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return -1;
    auto o = doc.object();
    if (!o.contains(imageDir_)) return -1;
    return o.value(imageDir_).toInt(-1);
}
