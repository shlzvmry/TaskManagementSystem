#include "mainwindow.h"
#include "database/database.h"
#include "widgets/watermarkwidget.h"
#include "models/taskmodel.h"
#include "models/inspirationmodel.h"
#include "dialogs/taskdialog.h"
#include "dialogs/recyclebindialog.h"
#include "dialogs/tagmanagerdialog.h"

#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QStyle>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QMessageBox>
#include <QShortcut>
#include <QMenuBar>
#include <QMenu>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , taskModel(nullptr)
    , inspirationModel(nullptr)
    , taskTableView(nullptr)
    , recycleBinDialog(nullptr)
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

    // 创建数据模型
    taskModel = new TaskModel(this);
    inspirationModel = new InspirationModel(this);

    // 创建回收站对话框
    recycleBinDialog = new RecycleBinDialog(this);
    recycleBinDialog->setTaskModel(taskModel);

    createWatermark();    // 创建水印
    setupSystemTray();    // 设置系统托盘
    setupUI();    // 初始化UI
    setupConnections();    // 设置信号连接

    // 设置回收站对话框的TaskModel
    if (recycleBinDialog && taskModel) {
        recycleBinDialog->setTaskModel(taskModel);
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");
    QAction *newAction = fileMenu->addAction("新建任务(&N)");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onAddTaskClicked);

    // 回收站菜单项
    QAction *recycleBinAction = fileMenu->addAction("回收站(&R)");
    recycleBinAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    connect(recycleBinAction, &QAction::triggered, this, &MainWindow::onRecycleBinClicked);

    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::quitApplication);

    // 编辑菜单
    QMenu *editMenu = menuBar->addMenu("编辑(&E)");
    QAction *editAction = editMenu->addAction("编辑任务(&E)");
    editAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditTaskClicked);

    QAction *deleteAction = editMenu->addAction("删除任务(&D)");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteTaskClicked);

    editMenu->addSeparator();
    QAction *refreshAction = editMenu->addAction("刷新(&R)");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshTasksClicked);

    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, []() {
        QMessageBox::about(nullptr, "关于",
                           "个人工作与任务管理系统\n\n"
                           "版本: 1.0.0\n"
                           "开发者: 谢静蕾\n"
                           "学号: 2023414300117\n\n"
                           "基于 Qt 6 开发的任务管理系统，支持任务管理、灵感记录、统计报表等功能。");
    });

    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 0);
    mainLayout->setSpacing(0);

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
    updateStatusBar(QString("就绪 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                        .arg(taskModel->getTaskCount())
                        .arg(taskModel->getCompletedCount())
                        .arg(taskModel->getDeletedTaskCount()));

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
    addBtn->setToolTip("添加新任务 (Ctrl+N)");

    QPushButton *editBtn = new QPushButton(taskTab);
    editBtn->setObjectName("editTaskBtn");
    editBtn->setIcon(QIcon(":/icons/edit_icon.png"));
    editBtn->setText("编辑任务");
    editBtn->setToolTip("编辑选中任务 (Ctrl+E)");

    QPushButton *deleteBtn = new QPushButton(taskTab);
    deleteBtn->setObjectName("deleteTaskBtn");
    deleteBtn->setIcon(QIcon(":/icons/delete_icon.png"));
    deleteBtn->setText("删除任务");
    deleteBtn->setToolTip("删除选中任务 (Delete)");

    // 回收站按钮
    QPushButton *recycleBinBtn = new QPushButton(taskTab);
    recycleBinBtn->setObjectName("recycleBinBtn");
    recycleBinBtn->setIcon(QIcon(":/icons/recycle_icon.png"));
    recycleBinBtn->setText("回收站");
    recycleBinBtn->setToolTip("打开回收站 (Ctrl+Shift+R)");

    QPushButton *refreshBtn = new QPushButton(taskTab);
    refreshBtn->setObjectName("refreshBtn");
    refreshBtn->setIcon(QIcon(":/icons/refresh_icon.png"));
    refreshBtn->setText("刷新");
    refreshBtn->setToolTip("刷新任务列表 (F5)");

    toolbarLayout->addWidget(addBtn);
    toolbarLayout->addWidget(editBtn);
    toolbarLayout->addWidget(deleteBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(recycleBinBtn);
    toolbarLayout->addWidget(refreshBtn);

    // 创建任务表格视图
    taskTableView = new QTableView(taskTab);
    taskTableView->setObjectName("taskTableView");
    setupTaskTableView();

    // 创建视图切换按钮
    QHBoxLayout *viewLayout = new QHBoxLayout();
    QPushButton *listViewBtn = new QPushButton("列表视图", taskTab);
    QPushButton *kanbanViewBtn = new QPushButton("看板视图", taskTab);
    QPushButton *calendarViewBtn = new QPushButton("日历视图", taskTab);

    // 新增：标签管理按钮
    QPushButton *tagManagerBtn = new QPushButton("标签管理", taskTab);
    tagManagerBtn->setObjectName("tagManagerBtn");
    tagManagerBtn->setIcon(QIcon(":/icons/edit_icon.png")); // 复用编辑图标

    listViewBtn->setObjectName("listViewBtn");
    kanbanViewBtn->setObjectName("kanbanViewBtn");
    calendarViewBtn->setObjectName("calendarViewBtn");

    viewLayout->addWidget(listViewBtn);
    viewLayout->addWidget(kanbanViewBtn);
    viewLayout->addWidget(calendarViewBtn);
    viewLayout->addStretch();
    viewLayout->addWidget(tagManagerBtn); // 放在最右侧

    layout->addLayout(toolbarLayout);
    layout->addWidget(taskTableView, 1);
    layout->addLayout(viewLayout);

    tabWidget->addTab(taskTab, "任务管理");
}

void MainWindow::setupTaskTableView()
{
    if (!taskTableView || !taskModel) return;

    taskTableView->setModel(taskModel);
    taskTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    taskTableView->setAlternatingRowColors(true);
    taskTableView->setSortingEnabled(true); // 确保开启排序
    taskTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 默认按截止时间排序
    taskTableView->sortByColumn(5, Qt::AscendingOrder);

    // 设置列宽
    taskTableView->horizontalHeader()->setStretchLastSection(true);
    taskTableView->setColumnWidth(0, 50);   // ID
    taskTableView->setColumnWidth(1, 200);  // 标题
    taskTableView->setColumnWidth(2, 100);  // 分类
    taskTableView->setColumnWidth(3, 80);   // 优先级
    taskTableView->setColumnWidth(4, 80);   // 状态
    taskTableView->setColumnWidth(5, 180);  // 截止时间
    taskTableView->setColumnWidth(6, 180);  // 提醒时间
    taskTableView->setColumnWidth(7, 140);  // 创建时间

    // 设置表头
    QHeaderView *header = taskTableView->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setHighlightSections(false);

    // 设置行高
    taskTableView->verticalHeader()->setDefaultSectionSize(30);
    taskTableView->verticalHeader()->setVisible(false);
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

void MainWindow::setupConnections()
{
    // 查找按钮并连接信号
    QPushButton *addBtn = findChild<QPushButton*>("addTaskBtn");
    if (addBtn) {
        connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddTaskClicked);
    }

    QPushButton *editBtn = findChild<QPushButton*>("editTaskBtn");
    if (editBtn) {
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::onEditTaskClicked);
    }

    QPushButton *deleteBtn = findChild<QPushButton*>("deleteTaskBtn");
    if (deleteBtn) {
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteTaskClicked);
    }

    QPushButton *refreshBtn = findChild<QPushButton*>("refreshBtn");
    if (refreshBtn) {
        connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshTasksClicked);
    }

    QPushButton *quickRecordBtn = findChild<QPushButton*>("quickRecordBtn");
    if (quickRecordBtn) {
        connect(quickRecordBtn, &QPushButton::clicked, this, &MainWindow::onQuickRecordClicked);
    }

    QPushButton *recycleBinBtn = findChild<QPushButton*>("recycleBinBtn");
    if (recycleBinBtn) {
        connect(recycleBinBtn, &QPushButton::clicked, this, &MainWindow::onRecycleBinClicked);
    }

    QPushButton *tagManagerBtn = findChild<QPushButton*>("tagManagerBtn");
    if (tagManagerBtn) {
        connect(tagManagerBtn, &QPushButton::clicked, this, &MainWindow::onTagManagerClicked);
    }

    new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), this, SLOT(onRecycleBinClicked()));

    // 连接表格双击事件
    if (taskTableView) {
        connect(taskTableView, &QTableView::doubleClicked, this, &MainWindow::onTaskDoubleClicked);
    }

    // 快捷键
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this, SLOT(onAddTaskClicked()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this, SLOT(onEditTaskClicked()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteTaskClicked()));
    new QShortcut(QKeySequence(Qt::Key_F5), this, SLOT(onRefreshTasksClicked()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I), this, SLOT(onQuickRecordClicked()));

    // 数据模型信号
    if (taskModel) {
        connect(taskModel, &TaskModel::taskAdded, this, [this](int taskId) {
            Q_UNUSED(taskId);
            updateStatusBar(QString("任务添加成功 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                                .arg(taskModel->getTaskCount())
                                .arg(taskModel->getCompletedCount())
                                .arg(taskModel->getDeletedTaskCount()));
        });

        connect(taskModel, &TaskModel::taskUpdated, this, [this](int taskId) {
            Q_UNUSED(taskId);
            updateStatusBar(QString("任务更新成功 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                                .arg(taskModel->getTaskCount())
                                .arg(taskModel->getCompletedCount())
                                .arg(taskModel->getDeletedTaskCount()));
        });

        connect(taskModel, &TaskModel::taskDeleted, this, [this](int taskId) {
            Q_UNUSED(taskId);
            updateStatusBar(QString("任务已移到回收站 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                                .arg(taskModel->getTaskCount())
                                .arg(taskModel->getCompletedCount())
                                .arg(taskModel->getDeletedTaskCount()));
        });

    }
}

// 回收站按钮点击
void MainWindow::onRecycleBinClicked()
{
    if (recycleBinDialog) {
        recycleBinDialog->refreshDeletedTasks();
        recycleBinDialog->exec();
    }
}

// 任务恢复后的处理
void MainWindow::onTaskRestored(int taskId)
{
    Q_UNUSED(taskId);
    updateStatusBar(QString("任务已恢复 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                        .arg(taskModel->getTaskCount())
                        .arg(taskModel->getCompletedCount())
                        .arg(taskModel->getDeletedTaskCount()));

    if (taskModel) {
        taskModel->refresh(false);
    }
}

// 任务永久删除后的处理
void MainWindow::onTaskPermanentlyDeleted(int taskId)
{
    Q_UNUSED(taskId);
    updateStatusBar(QString("任务已永久删除 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                        .arg(taskModel->getTaskCount())
                        .arg(taskModel->getCompletedCount())
                        .arg(taskModel->getDeletedTaskCount()));
}

void MainWindow::onAddTaskClicked()
{
    TaskDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap taskData = dialog.getTaskData();

        if (taskModel->addTask(taskData)) {
            updateStatusBar("新任务添加成功");
        } else {
            QMessageBox::warning(this, "错误", "添加任务失败");
        }
    }
}

void MainWindow::onEditTaskClicked()
{
    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        QMessageBox::warning(this, "提示", "请先选择一个任务");
        return;
    }

    QVariantMap taskData = taskModel->getTask(taskId);
    if (taskData.isEmpty()) {
        QMessageBox::warning(this, "错误", "获取任务信息失败");
        return;
    }

    TaskDialog dialog(taskData, this);

    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap updatedData = dialog.getTaskData();

        if (taskModel->updateTask(taskId, updatedData)) {
            updateStatusBar("任务更新成功");
        } else {
            QMessageBox::warning(this, "错误", "更新任务失败");
        }
    }
}

void MainWindow::onDeleteTaskClicked()
{
    int taskId = getSelectedTaskId();
    if (taskId == -1) {
        QMessageBox::warning(this, "提示", "请先选择一个任务");
        return;
    }

    QVariantMap taskData = taskModel->getTask(taskId);
    QString taskTitle = taskData.value("title").toString();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("确定要将任务 '%1' 移动到回收站吗？\n(可以在回收站中恢复或永久删除)")
                                      .arg(taskTitle),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 使用软删除（移动到回收站）
        if (taskModel->deleteTask(taskId, true)) {
            updateStatusBar(QString("任务 '%1' 已移动到回收站").arg(taskTitle));
        } else {
            QMessageBox::warning(this, "错误", "删除任务失败");
        }
    }
}

void MainWindow::onRefreshTasksClicked()
{
    if (taskModel) {
        taskModel->refresh();
        updateStatusBar("任务列表已刷新");
    }
}

void MainWindow::onTagManagerClicked()
{
    TagManagerDialog dialog(this);
    dialog.exec();

    // 标签可能被修改，刷新任务列表以更新显示
    if (taskModel) {
        taskModel->refresh();
    }
}

void MainWindow::onTaskDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    onEditTaskClicked();
}

int MainWindow::getSelectedTaskId() const
{
    if (!taskTableView || !taskTableView->selectionModel()->hasSelection()) {
        return -1;
    }

    QModelIndex index = taskTableView->selectionModel()->selectedRows().first();
    return taskModel->data(index, TaskModel::IdRole).toInt();
}

void MainWindow::onQuickRecordClicked()
{
    QMessageBox::information(this, "功能开发中", "快速记录灵感功能将在灵感记录模块实现");
    updateStatusBar("准备记录灵感...");
}

void MainWindow::updateStatusBar(const QString &message)
{
    if (statusBarWidget) {
        statusBarWidget->showMessage(message);
    }
}

void MainWindow::createWatermark()
{
    // 创建水印部件
    WatermarkWidget *watermark = new WatermarkWidget("谢静蕾 2023414300117", this);

    connect(this, &MainWindow::windowTitleChanged, watermark, [watermark, this]() {
        watermark->setGeometry(0, 0, this->width(), this->height());
    });

    watermark->lower();
    watermark->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    watermark->setFocusPolicy(Qt::NoFocus);
    watermark->setGeometry(0, 0, width(), height());
    watermark->update();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    WatermarkWidget *watermark = findChild<WatermarkWidget*>();
    if (watermark) {
        watermark->setGeometry(0, 0, width(), height());
        watermark->update();
    }
}

void MainWindow::loadStyleSheet()
{
    QString styleSheet;
    // 定义基础路径，方便统一管理
    QString basePath = "D:/Qt/TaskManagementSystem/styles/";

    // 1. 加载主窗口样式
    QString mainPath = basePath + "mainwindow.qss";
    QFile mainFile(mainPath);
    if (mainFile.open(QFile::ReadOnly | QFile::Text)) {
        styleSheet += QLatin1String(mainFile.readAll());
        mainFile.close();
    } else {
        qDebug() << "主窗口样式加载失败：" << mainPath;
    }

    // 2. 加载对话框样式
    QString dialogPath = basePath + "dialog.qss";
    QFile dialogFile(dialogPath);
    if (dialogFile.open(QFile::ReadOnly | QFile::Text)) {
        styleSheet += "\n" + QLatin1String(dialogFile.readAll());
        dialogFile.close();
    } else {
        qDebug() << "对话框样式加载失败：" << dialogPath;
    }

    // 3. 加载控件样式
    QString widgetPath = basePath + "widget.qss";
    QFile widgetFile(widgetPath);
    if (widgetFile.open(QFile::ReadOnly | QFile::Text)) {
        styleSheet += "\n" + QLatin1String(widgetFile.readAll());
        widgetFile.close();
    } else {
        qDebug() << "控件样式加载失败：" << widgetPath;
    }

    // 应用样式表
    if (!styleSheet.isEmpty()) {
        qApp->setStyleSheet("");
        qApp->setStyleSheet(styleSheet);
        qDebug() << "样式表应用成功";
    } else {
        qDebug() << "样式表为空，使用默认样式";
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
