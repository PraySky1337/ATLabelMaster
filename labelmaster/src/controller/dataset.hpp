#pragma once
#include <QFileSystemModel>
// dataset_manager.hpp
#include <QString>

namespace controller {
class DatasetManager {
public:
    static DatasetManager& instance();

    // 记住/读取数据集保存目录（images/labels 会在保存时自动创建）
    void setSaveDir(const QString& path);
    QString saveDir() const;

    // 当前图片目录（用于每个图片目录的进度记忆）
    void setImageDir(const QString& imageDir);
    QString imageDir() const;

    // 进度：记忆当前索引（与 imageDir 绑定）
    void saveProgress(int currentIndex);
    int loadProgress() const; // 若无记录则返回 -1

private:
    DatasetManager();
    QString cfgPath() const;  // ~/.atlabelmaster/config.json

    // 内部状态缓存
    QString saveDir_;
    QString imageDir_;
};
} // namespace controller