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
#include <QTimer>

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
    void onAddTaskClicked();
    void onEditTaskClicked();
    void onDeleteTaskClicked();
    void onRefreshTasksClicked();
    void onTaskDoubleClicked(const QModelIndex &index);
     void onRecycleBinClicked();

    void onQuickRecordClicked();
     void onCalendarDateClicked(const QDate &date);

    void onTaskRestored(int taskId);
    void onTaskPermanentlyDeleted(int taskId);

    void onTagManagerClicked();
    void onEditTask(int taskId);

    void onCalendarShowInspirations(const QDate &date);
    void onCalendarShowTasks(const QDate &date);

private:
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QTabWidget *tabWidget;
    QStatusBar *statusBarWidget;

    TaskModel *taskModel;
    InspirationModel *inspirationModel;
    class StatisticModel *statisticModel;

    QTableView *taskTableView;
    QSplitter *taskSplitter;
    TaskTableView *uncompletedTableView;
    TaskTableView *completedTableView;

    class KanbanView *kanbanView;
    class CalendarView *calendarView;
    class StatisticView *statisticView;
    class QStackedWidget *viewStack;

    class QComboBox *filterCategoryCombo;
    class QComboBox *filterPriorityCombo;
    class QLineEdit *searchEdit;
    class QPushButton *kanbanGroupBtn;

    RecycleBinDialog *recycleBinDialog;
    QTimer *overdueCheckTimer;

    TaskFilterModel *uncompletedProxyModel;
    TaskFilterModel *completedProxyModel;

    void setupSystemTray();
    void setupUI();
    void createWatermark();
    void loadStyleSheet();
    void setupConnections();

    void createTaskTab();
    void createInspirationTab();
    void createStatisticTab();
    void createSettingTab();

    void updateStatusBar(const QString &message);
    int getSelectedTaskId() const;
};
#endif // MAINWINDOW_H
