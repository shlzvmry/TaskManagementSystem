#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTabWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QTableView>
#include <QSplitter>

// 前向声明
class TaskModel;
class InspirationModel;
class WatermarkWidget;
class TaskDialog;
class RecycleBinDialog;
class TaskFilterModel;
class TaskTableView;

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
     void onCalendarDateClicked(const QDate &date);

    //回收站相关
    void onTaskRestored(int taskId);
    void onTaskPermanentlyDeleted(int taskId);

    //标签管理
    void onTagManagerClicked();
    void onEditTask(int taskId);

    // 日历交互
    void onCalendarShowInspirations(const QDate &date);
    void onCalendarShowTasks(const QDate &date);

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
    QSplitter *taskSplitter;
    TaskTableView *uncompletedTableView; // 原 QTableView *uncompletedTableView;
    TaskTableView *completedTableView;

    // 新增视图
    class KanbanView *kanbanView;
    class CalendarView *calendarView;
    class QStackedWidget *viewStack;

    // 过滤器控件
    class QComboBox *filterCategoryCombo;
    class QComboBox *filterPriorityCombo;
    class QLineEdit *searchEdit;
    class QPushButton *kanbanGroupBtn;

    //对话框
    RecycleBinDialog *recycleBinDialog;

    // 代理模型
    TaskFilterModel *uncompletedProxyModel;
    TaskFilterModel *completedProxyModel;

    // 初始化函数
    void setupSystemTray();
    void setupUI();
    void createWatermark();
    void loadStyleSheet();
    void setupConnections();

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
