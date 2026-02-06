#pragma once
#include "service/file.hpp"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <memory>
#include <qaction.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
class QImage;
class QKeyEvent;
class QDragEnterEvent;
class QDropEvent;
class QCloseEvent;
QT_END_NAMESPACE

namespace ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // —— 配置项（可选）——
    void enableDragDrop(bool on = true);
    void setLogTimestampEnabled(bool on = true);
    auto ui() { return ui_.get(); }

signals:
    // —— 用户输出（语义化）——
    void sigOpenFolderRequested();
    void sigSaveRequested();
    void sigPrevRequested();
    void sigNextRequested();
    void sigHistEqRequested();
    void sigDeleteRequested();
    void sigSmartAnnotateRequested();
    void sigSettingsRequested();
    void sigDatasetRequested();
    void sigGetStasRequested(int colorId, int classId, int sizeId);
    void sigAnnotationFiltersChanged(int colorId, int classId, int sizeId);
    void sigFileActivated(const QModelIndex&);
    void sigDroppedPaths(const QStringList&);
    void sigKeyCommand(const QString&);
    void sigStasUpdateRequested(const int& targetCount, const int& fileCount); // 统计信息输出
    // —— 类别相关输出 ——
    void sigLabelContentChanged(const QString& content); // 标签文件内容变化
    void sigStasGetted(const int& targetCount, const int& fileCount);

    // FILE：通知 service 侧刷新索引（可选但推荐）
    void sigTreeModelReplaced(QAbstractItemModel* model);
    void sigTreeRootChanged(const QModelIndex& root);

public slots:
    // —— 外部输入（更新 UI）——
    void showSettingDialog();
    void showDatasetDialog();
    void showStasDialog();
    void showImage(const QImage& img);
    void appendLog(const QString& line);
    void setFileModel(QAbstractItemModel* model);
    void setCurrentIndex(const QModelIndex& idx);
    void setStatus(const QString& msg, int ms = 3000);
    void setBusy(bool on);
    void setUiEnabled(bool on);
    void setRoot(const QModelIndex& idx);

    // —— 标签内容查看器 ——
    void setLabelContent(const QString& content);

    // —— 标注过滤器 ——
    void updateAnnotationFilters();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    void setupActions();
    void wireButtonsToActions();
    bool textInputHasFocus() const;

private:
    std::unique_ptr<Ui::MainWindow> ui_;
    bool logTimestamp_    = true;
    bool dragDropEnabled_ = true;

    // 当前选中的类别（用于键盘快捷键1-9选择）
    QString currentClass_ = QStringLiteral("G");
};

} // namespace ui
