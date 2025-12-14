#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTabWidget>
#include <QStatusBar>
#include <QVBoxLayout>

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

private:
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QTabWidget *tabWidget;
    QStatusBar *statusBarWidget;

    void setupSystemTray();
    void setupUI();
    void createWatermark();
    void loadStyleSheet();

    // 创建各个Tab页
    void createTaskTab();
    void createInspirationTab();
    void createStatisticTab();
    void createSettingTab();
};
#endif // MAINWINDOW_H
