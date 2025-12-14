#include "mainwindow.h"
#include "database/database.h"
#include "widgets/watermarkwidget.h"
#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QStyle>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 初始化数据库
    Database::instance().initDatabase();

    // 加载样式表
    loadStyleSheet();

    // 设置窗口大小和位置
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int width = screenGeometry.width() * 3 / 4;
    int height = screenGeometry.height() * 3 / 4;
    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;

    setGeometry(x, y, width, height);
    setWindowTitle("个人工作与任务管理系统");

    // 创建水印 - 必须先于其他控件
    createWatermark();

    // 设置系统托盘
    setupSystemTray();

    // 初始化UI
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 创建标题栏
    QLabel *titleLabel = new QLabel("个人工作与任务管理系统", centralWidget);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 创建Tab控件
    tabWidget = new QTabWidget(centralWidget);
    tabWidget->setObjectName("mainTabWidget");

    // 创建各个Tab页
    createTaskTab();
    createInspirationTab();
    createStatisticTab();
    createSettingTab();

    mainLayout->addWidget(tabWidget);

    // 创建状态栏
    statusBarWidget = new QStatusBar(this);
    setStatusBar(statusBarWidget);
    statusBarWidget->showMessage("就绪");

    // 添加底部信息
    QLabel *infoLabel = new QLabel("开发者：谢静蕾 | 学号：2023414300117", centralWidget);
    infoLabel->setObjectName("infoLabel");
    infoLabel->setAlignment(Qt::AlignRight);
    mainLayout->addWidget(infoLabel);
}

void MainWindow::createTaskTab()
{
    QWidget *taskTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(taskTab);

    // 创建工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    // 使用图标的按钮
    QPushButton *addBtn = new QPushButton(taskTab);
    addBtn->setObjectName("addTaskBtn");
    addBtn->setIcon(QIcon(":/icons/add_icon.png"));
    addBtn->setText("添加任务");
    addBtn->setToolTip("添加新任务");

    QPushButton *editBtn = new QPushButton(taskTab);
    editBtn->setObjectName("editTaskBtn");
    editBtn->setIcon(QIcon(":/icons/edit_icon.png"));
    editBtn->setText("编辑任务");
    editBtn->setToolTip("编辑选中任务");

    QPushButton *deleteBtn = new QPushButton(taskTab);
    deleteBtn->setObjectName("deleteTaskBtn");
    deleteBtn->setIcon(QIcon(":/icons/delete_icon.png"));
    deleteBtn->setText("删除任务");
    deleteBtn->setToolTip("删除选中任务");

    QPushButton *refreshBtn = new QPushButton(taskTab);
    refreshBtn->setObjectName("refreshBtn");
    refreshBtn->setIcon(QIcon(":/icons/refresh_icon.png"));
    refreshBtn->setText("刷新");
    refreshBtn->setToolTip("刷新任务列表");

    toolbarLayout->addWidget(addBtn);
    toolbarLayout->addWidget(editBtn);
    toolbarLayout->addWidget(deleteBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(refreshBtn);

    // 创建任务列表区域
    QLabel *listLabel = new QLabel("任务列表将显示在这里", taskTab);
    listLabel->setObjectName("taskListLabel");
    listLabel->setAlignment(Qt::AlignCenter);
    listLabel->setMinimumHeight(400);

    // 创建视图切换按钮
    QHBoxLayout *viewLayout = new QHBoxLayout();
    QPushButton *listViewBtn = new QPushButton("列表视图", taskTab);
    QPushButton *kanbanViewBtn = new QPushButton("看板视图", taskTab);
    QPushButton *calendarViewBtn = new QPushButton("日历视图", taskTab);

    listViewBtn->setObjectName("listViewBtn");
    kanbanViewBtn->setObjectName("kanbanViewBtn");
    calendarViewBtn->setObjectName("calendarViewBtn");

    viewLayout->addWidget(listViewBtn);
    viewLayout->addWidget(kanbanViewBtn);
    viewLayout->addWidget(calendarViewBtn);
    viewLayout->addStretch();

    layout->addLayout(toolbarLayout);
    layout->addWidget(listLabel, 1);
    layout->addLayout(viewLayout);

    tabWidget->addTab(taskTab, "任务管理");
}

void MainWindow::setupSystemTray()
{
    trayIcon = new QSystemTrayIcon(this);

    // 使用资源中的托盘图标
    QIcon trayIconResource(":/icons/tray_icon.png");
    if (trayIconResource.isNull()) {
        qDebug() << "托盘图标加载失败，使用默认图标";
        trayIcon->setIcon(QIcon::fromTheme("calendar"));
    } else {
        trayIcon->setIcon(trayIconResource);
    }

    trayMenu = new QMenu(this);
    trayMenu->addAction("显示主窗口", this, &MainWindow::showMainWindow);
    trayMenu->addSeparator();
    trayMenu->addAction("退出", this, &MainWindow::quitApplication);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
}

void MainWindow::createInspirationTab()
{
    QWidget *inspirationTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(inspirationTab);

    // 快速记录区域
    QHBoxLayout *quickRecordLayout = new QHBoxLayout();
    QLabel *recordLabel = new QLabel("快速记录灵感：", inspirationTab);
    QPushButton *quickRecordBtn = new QPushButton("+ 记录灵感", inspirationTab);
    quickRecordBtn->setObjectName("quickRecordBtn");

    quickRecordLayout->addWidget(recordLabel);
    quickRecordLayout->addWidget(quickRecordBtn);
    quickRecordLayout->addStretch();

    // 灵感列表区域
    QLabel *listLabel = new QLabel("灵感记录将显示在这里", inspirationTab);
    listLabel->setObjectName("inspirationListLabel");
    listLabel->setAlignment(Qt::AlignCenter);
    listLabel->setMinimumHeight(400);

    layout->addLayout(quickRecordLayout);
    layout->addWidget(listLabel, 1);

    tabWidget->addTab(inspirationTab, "灵感记录");
}

void MainWindow::createStatisticTab()
{
    QWidget *statisticTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(statisticTab);

    QLabel *statLabel = new QLabel("统计分析将显示在这里", statisticTab);
    statLabel->setObjectName("statisticLabel");
    statLabel->setAlignment(Qt::AlignCenter);
    statLabel->setMinimumHeight(400);

    layout->addWidget(statLabel);

    tabWidget->addTab(statisticTab, "统计分析");
}

void MainWindow::createSettingTab()
{
    QWidget *settingTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(settingTab);

    QLabel *settingLabel = new QLabel("系统设置将显示在这里", settingTab);
    settingLabel->setObjectName("settingLabel");
    settingLabel->setAlignment(Qt::AlignCenter);
    settingLabel->setMinimumHeight(400);

    layout->addWidget(settingLabel);

    tabWidget->addTab(settingTab, "系统设置");
}

void MainWindow::createWatermark()
{
    // 创建水印部件
    WatermarkWidget *watermark = new WatermarkWidget("谢静蕾 2023414300117", this);

    // 确保水印覆盖整个窗口
    connect(this, &MainWindow::windowTitleChanged, watermark, [watermark, this]() {
        watermark->setGeometry(0, 0, this->width(), this->height());
    });

    // 将水印置于底层
    watermark->lower();

    // 设置水印为底层且不接收鼠标事件
    watermark->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    watermark->setFocusPolicy(Qt::NoFocus);

    // 初始大小
    watermark->setGeometry(0, 0, width(), height());

    // 强制重绘
    watermark->update();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 查找并更新水印大小
    WatermarkWidget *watermark = findChild<WatermarkWidget*>();
    if (watermark) {
        watermark->setGeometry(0, 0, width(), height());
        watermark->update();
    }
}

void MainWindow::loadStyleSheet()
{
    // 加载主窗口样式
    QString absolutePath = "D:/Qt/TaskManagementSystem/styles/mainwindow.qss";
    QFile file(absolutePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        if (!styleSheet.isEmpty()) {
            qApp->setStyleSheet(styleSheet);
            file.close();
            qDebug() << "样式表从绝对路径加载成功：" << absolutePath;
            return;
        } else {
            qDebug() << "样式表文件为空";
        }
    } else {
        qDebug() << "无法从绝对路径加载样式表：" << absolutePath;
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showMainWindow();
    }
}

void MainWindow::showMainWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::quitApplication()
{
    qApp->quit();
}
