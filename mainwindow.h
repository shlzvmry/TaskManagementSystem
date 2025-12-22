#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTabWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QTableView>

// 前向声明
class TaskModel;
class InspirationModel;
class WatermarkWidget;
class TaskDialog;
class RecycleBinDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showMainWindow();
    void quitApplication();

    // 任务相关槽函数
    void onAddTaskClicked();
    void onEditTaskClicked();
    void onDeleteTaskClicked();
    void onRefreshTasksClicked();
    void onTaskDoubleClicked(const QModelIndex &index);
     void onRecycleBinClicked();

    // 灵感相关槽函数
    void onQuickRecordClicked();

    //回收站相关
    void onTaskRestored(int taskId);
    void onTaskPermanentlyDeleted(int taskId);

private:
    // UI组件
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QTabWidget *tabWidget;
    QStatusBar *statusBarWidget;

    // 数据模型
    TaskModel *taskModel;
    InspirationModel *inspirationModel;

    // 视图组件
    QTableView *taskTableView;

    //对话框
    RecycleBinDialog *recycleBinDialog;

    // 初始化函数
    void setupSystemTray();
    void setupUI();
    void createWatermark();
    void loadStyleSheet();
    void setupConnections();
    void setupTaskTableView();

    // 创建各个Tab页
    void createTaskTab();
    void createInspirationTab();
    void createStatisticTab();
    void createSettingTab();

    // 工具函数
    void updateStatusBar(const QString &message);
    int getSelectedTaskId() const;
};
#endif // MAINWINDOW_H
