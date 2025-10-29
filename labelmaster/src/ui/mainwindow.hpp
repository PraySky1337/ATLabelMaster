#pragma once
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QAbstractItemModel;
class QModelIndex;
class QImage;
class QKeyEvent;
class QDragEnterEvent;
class QDropEvent;
class QCloseEvent;
class QStringListModel;
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
    void sigAnnotateRequested();
    void sigFileActivated(const QModelIndex&);
    void sigDroppedPaths(const QStringList&);
    void sigKeyCommand(const QString&);

    // —— 类别相关输出 ——
    void sigClassSelected(const QString& name);  // 选中类别时发出

public slots:
    // —— 外部输入（更新 UI）——
    void showImage(const QImage& img);
    void appendLog(const QString& line);
    void setFileModel(QAbstractItemModel* model);
    void setCurrentIndex(const QModelIndex& idx);
    void setStatus(const QString& msg, int ms = 2000);
    void setBusy(bool on);
    void setUiEnabled(bool on);
    void setRoot(const QModelIndex& idx);

    // —— 类别列表 —— 
    void setClassList(const QStringList& names);
    void setCurrentClass(const QString& name);    // 可选：代码里直接选中某类

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    void setupActions();
    void wireButtonsToActions();
    bool textInputHasFocus() const;

    // 类别视图初始化/行为
    void setupClassListView();
    void onClassCurrentChanged(const QModelIndex& current);

private:
    std::unique_ptr<Ui::MainWindow> ui_;
    bool logTimestamp_ = true;
    bool dragDropEnabled_ = true;

    // 类别
    QStringListModel* clsModel_ = nullptr;
    QString           currentClass_;
};

} // namespace ui
