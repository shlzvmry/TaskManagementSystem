#include "mainwindow.h"
#include "database/database.h"
#include "widgets/watermarkwidget.h"
#include "models/taskmodel.h"
#include "models/inspirationmodel.h"
#include "dialogs/taskdialog.h"
#include "dialogs/recyclebindialog.h"
#include "dialogs/tagmanagerdialog.h"
#include "models/taskfiltermodel.h"
#include "widgets/comboboxdelegate.h"
#include "views/kanbanview.h"
#include "views/calenderview.h"
#include "views/tasktableview.h"
#include "views/inspirationview.h"
#include "dialogs/inspirationdialog.h"

#include <QStackedWidget>
#include <QComboBox>
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
#include <QSplitter>
#include <QLineEdit>
#include <QButtonGroup>
#include <QShortcut>
#include <QKeySequence>
#include <QListWidget>
#include <QMouseEvent>
#include <QTableWidget>


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

    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    QPushButton *addBtn = new QPushButton("添加", taskTab);
    addBtn->setObjectName("addTaskBtn");
    addBtn->setIcon(QIcon(":/icons/add_icon.png"));

    QPushButton *editBtn = new QPushButton("编辑", taskTab);
    editBtn->setObjectName("editTaskBtn");
    editBtn->setIcon(QIcon(":/icons/edit_icon.png"));

    QPushButton *deleteBtn = new QPushButton("删除", taskTab);
    deleteBtn->setObjectName("deleteTaskBtn");
    deleteBtn->setIcon(QIcon(":/icons/delete_icon.png"));

    toolbarLayout->addWidget(addBtn);
    toolbarLayout->addWidget(editBtn);
    toolbarLayout->addWidget(deleteBtn);

    toolbarLayout->addSpacing(20);
    toolbarLayout->addWidget(new QLabel("过滤:", taskTab));

    filterCategoryCombo = new QComboBox(taskTab);
    filterCategoryCombo->setObjectName("filterCategoryCombo");
    filterCategoryCombo->addItem("所有分类", -1);
    QList<QVariantMap> cats = Database::instance().getAllCategories();
    for(const auto &cat : cats) {
        filterCategoryCombo->addItem(cat["name"].toString(), cat["id"]);
    }

    filterPriorityCombo = new QComboBox(taskTab);
    filterPriorityCombo->setObjectName("filterPriorityCombo");
    filterPriorityCombo->addItem("所有优先级", -1);
    filterPriorityCombo->addItem("紧急", 0);
    filterPriorityCombo->addItem("重要", 1);
    filterPriorityCombo->addItem("普通", 2);
    filterPriorityCombo->addItem("不急", 3);

    searchEdit = new QLineEdit(taskTab);
    searchEdit->setPlaceholderText("搜索任务...");
    searchEdit->setFixedWidth(190);

    toolbarLayout->addWidget(filterCategoryCombo);
    toolbarLayout->addWidget(filterPriorityCombo);
    toolbarLayout->addWidget(searchEdit);

    toolbarLayout->addStretch();

    QPushButton *recycleBinBtn = new QPushButton("回收站", taskTab);
    recycleBinBtn->setObjectName("taskRecycleBinBtn");
    recycleBinBtn->setIcon(QIcon(":/icons/recycle_icon.png"));

    QPushButton *tagManagerBtn = new QPushButton("标签管理", taskTab);
    tagManagerBtn->setObjectName("taskTagManagerBtn");
    tagManagerBtn->setIcon(QIcon(":/icons/edit_icon.png"));

    QPushButton *refreshBtn = new QPushButton("刷新", taskTab);
    refreshBtn->setObjectName("taskRefreshBtn");
    refreshBtn->setIcon(QIcon(":/icons/refresh_icon.png"));

    toolbarLayout->addWidget(recycleBinBtn);
    toolbarLayout->addWidget(tagManagerBtn);
    toolbarLayout->addWidget(refreshBtn);

    viewStack = new QStackedWidget(taskTab);

    // 视图1: 列表视图
    QWidget *listViewWidget = new QWidget();
    QVBoxLayout *listLayout = new QVBoxLayout(listViewWidget);
    listLayout->setContentsMargins(0,0,0,0);

    taskSplitter = new QSplitter(Qt::Vertical, listViewWidget);
    taskSplitter->setHandleWidth(1);
    taskSplitter->setStyleSheet("QSplitter::handle { background-color: #3d3d3d; }");

    uncompletedProxyModel = new TaskFilterModel(this);
    uncompletedProxyModel->setSourceModel(taskModel);
    uncompletedProxyModel->setFilterMode(TaskFilterModel::FilterUncompleted);

    completedProxyModel = new TaskFilterModel(this);
    completedProxyModel->setSourceModel(taskModel);
    completedProxyModel->setFilterMode(TaskFilterModel::FilterCompleted);

    uncompletedTableView = new TaskTableView(taskSplitter);
    uncompletedTableView->setObjectName("uncompletedTableView");
    uncompletedTableView->setModel(uncompletedProxyModel); // setModel 会自动配置列宽和代理

    // 连接双击编辑信号
    connect(uncompletedTableView, &TaskTableView::editTaskRequested,
            this, &MainWindow::onEditTask);

    QWidget *bottomContainer = new QWidget(taskSplitter);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomContainer);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    // 创建醒目的横条按钮
    QPushButton *separatorBtn = new QPushButton(bottomContainer);
    separatorBtn->setObjectName("completedSeparatorBtn");
    separatorBtn->setCursor(Qt::PointingHandCursor);
    separatorBtn->setFixedHeight(7);
    separatorBtn->setFlat(true);

    // 创建已完成列表
    completedTableView = new TaskTableView(bottomContainer);
    completedTableView->setObjectName("completedTableView");
    completedTableView->setModel(completedProxyModel);

    // 连接双击编辑信号
    connect(completedTableView, &TaskTableView::editTaskRequested,
            this, &MainWindow::onEditTask);

    // 已完成列表的特殊设置 (隐藏表头等)
    completedTableView->horizontalHeader()->hide();
    completedTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    completedTableView->setFrameShape(QFrame::NoFrame);

    // 点击横条切换表头显示
    connect(separatorBtn, &QPushButton::clicked, [this]() {
        bool isVisible = completedTableView->horizontalHeader()->isVisible();
        completedTableView->horizontalHeader()->setVisible(!isVisible);
    });

    bottomLayout->addWidget(separatorBtn);
    bottomLayout->addWidget(completedTableView);

    // 将未完成列表和底部容器加入分割器
    taskSplitter->addWidget(uncompletedTableView);
    taskSplitter->addWidget(bottomContainer);
    taskSplitter->setStretchFactor(0, 7);
    taskSplitter->setStretchFactor(1, 3);

    listLayout->addWidget(taskSplitter);

    // 视图2: 看板视图
    kanbanView = new KanbanView(taskTab);
    kanbanView->setModel(taskModel);

    // 连接看板视图的编辑信号
    connect(kanbanView, &KanbanView::editTaskRequested, this, &MainWindow::onEditTask);

    // 视图3: 日历视图
    calendarView = new CalendarView(taskTab);
    calendarView->setTaskModel(taskModel);
    calendarView->setInspirationModel(inspirationModel);

    // 添加到 Stack
    viewStack->addWidget(listViewWidget); // Index 0
    viewStack->addWidget(kanbanView);     // Index 1
    viewStack->addWidget(calendarView);   // Index 2

    // 连接日历点击信号
    connect(calendarView, &CalendarView::showInspirations, this, &MainWindow::onCalendarShowInspirations);
    connect(calendarView, &CalendarView::showTasks, this, &MainWindow::onCalendarShowTasks);

    // 底部视图切换栏
    QHBoxLayout *bottomBarLayout = new QHBoxLayout();

    // 1. 左侧容器 (固定宽度，确保与右侧占位符对称)
    QWidget *leftContainer = new QWidget(taskTab);
    leftContainer->setFixedWidth(110);
    QHBoxLayout *leftContainerLayout = new QHBoxLayout(leftContainer);
    leftContainerLayout->setContentsMargins(0, 0, 0, 0);

    // 看板分组切换按钮 (放入左侧容器)
    kanbanGroupBtn = new QPushButton("分组: 状态", leftContainer);
    kanbanGroupBtn->setObjectName("kanbanGroupBtn");
    kanbanGroupBtn->setCursor(Qt::PointingHandCursor);
    // 按钮填满容器或自适应，容器本身限制了宽度
    kanbanGroupBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    kanbanGroupBtn->setVisible(false); // 默认隐藏

    leftContainerLayout->addWidget(kanbanGroupBtn);

    connect(kanbanGroupBtn, &QPushButton::clicked, this, [this](){
        if (!kanbanView) return;
        if (kanbanView->getGroupMode() == KanbanView::GroupByStatus) {
            kanbanView->setGroupMode(KanbanView::GroupByPriority);
            kanbanGroupBtn->setText("分组: 优先级");
        } else {
            kanbanView->setGroupMode(KanbanView::GroupByStatus);
            kanbanGroupBtn->setText("分组: 状态");
        }
    });

    // 2. 中间：视图切换按钮组
    QButtonGroup *viewGroup = new QButtonGroup(taskTab);
    QPushButton *listViewBtn = new QPushButton("列表视图", taskTab);
    listViewBtn->setCheckable(true);
    listViewBtn->setChecked(true);
    listViewBtn->setObjectName("listViewBtn");

    QPushButton *kanbanViewBtn = new QPushButton("看板视图", taskTab);
    kanbanViewBtn->setCheckable(true);
    kanbanViewBtn->setObjectName("kanbanViewBtn");

    QPushButton *calendarViewBtn = new QPushButton("日历视图", taskTab);
    calendarViewBtn->setCheckable(true);
    calendarViewBtn->setObjectName("calendarViewBtn");

    viewGroup->addButton(listViewBtn, 0);
    viewGroup->addButton(kanbanViewBtn, 1);
    viewGroup->addButton(calendarViewBtn, 2);

    QHBoxLayout *centerBtnLayout = new QHBoxLayout();
    centerBtnLayout->addWidget(listViewBtn);
    centerBtnLayout->addWidget(kanbanViewBtn);
    centerBtnLayout->addWidget(calendarViewBtn);

    // 3. 右侧：占位控件 (宽度与左侧容器一致，保证中间绝对居中)
    QWidget *dummyRight = new QWidget(taskTab);
    dummyRight->setFixedWidth(110);

    // 组装底部栏： [左侧容器] [弹簧] [中间按钮组] [弹簧] [右侧占位]
    bottomBarLayout->addWidget(leftContainer);
    bottomBarLayout->addStretch();
    bottomBarLayout->addLayout(centerBtnLayout);
    bottomBarLayout->addStretch();
    bottomBarLayout->addWidget(dummyRight);

    // 连接视图切换
    connect(viewGroup, &QButtonGroup::idClicked, this, [this](int id){
        viewStack->setCurrentIndex(id);
        if (kanbanGroupBtn) {
            kanbanGroupBtn->setVisible(id == 1);
        }
    });

    // 连接过滤器
    auto updateFilters = [this]() {
        int catId = filterCategoryCombo->currentData().toInt();
        int pri = filterPriorityCombo->currentData().toInt();
        QString text = searchEdit->text();

        uncompletedProxyModel->setFilterCategory(catId);
        uncompletedProxyModel->setFilterPriority(pri);
        uncompletedProxyModel->setFilterText(text);

        completedProxyModel->setFilterCategory(catId);
        completedProxyModel->setFilterPriority(pri);
        completedProxyModel->setFilterText(text);
    };

    connect(filterCategoryCombo, &QComboBox::currentIndexChanged, this, updateFilters);
    connect(filterPriorityCombo, &QComboBox::currentIndexChanged, this, updateFilters);
    connect(searchEdit, &QLineEdit::textChanged, this, updateFilters);

    // 布局组装
    layout->addLayout(toolbarLayout);
    layout->addWidget(viewStack, 1);
    layout->addLayout(bottomBarLayout);

    tabWidget->addTab(taskTab, "任务管理");
}

void MainWindow::setupSystemTray()
{
    trayIcon = new QSystemTrayIcon(this);
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
    layout->setContentsMargins(0, 0, 0, 0);

    // 实例化灵感视图
    InspirationView *inspirationView = new InspirationView(inspirationTab);
    inspirationView->setModel(inspirationModel);
    inspirationView->setTaskModel(taskModel);

    connect(inspirationView, &InspirationView::showInspirationsRequested, this, &MainWindow::onCalendarShowInspirations);
    connect(inspirationView, &InspirationView::showTasksRequested, this, &MainWindow::onCalendarShowTasks);

    layout->addWidget(inspirationView);

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

    QPushButton *quickRecordBtn = findChild<QPushButton*>("quickRecordBtn");
    if (quickRecordBtn) {
        connect(quickRecordBtn, &QPushButton::clicked, this, &MainWindow::onQuickRecordClicked);
    }

    QPushButton *recycleBinBtn = findChild<QPushButton*>("taskRecycleBinBtn");
    if (recycleBinBtn) {
        connect(recycleBinBtn, &QPushButton::clicked, this, &MainWindow::onRecycleBinClicked);
    }

    QPushButton *tagManagerBtn = findChild<QPushButton*>("taskTagManagerBtn");
    if (tagManagerBtn) {
        connect(tagManagerBtn, &QPushButton::clicked, this, &MainWindow::onTagManagerClicked);
    }

    QPushButton *refreshBtn = findChild<QPushButton*>("taskRefreshBtn");
    if (refreshBtn) {
        connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshTasksClicked);
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

void MainWindow::onEditTask(int taskId)
{
    if (taskId <= 0) return;

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
    // 修改类型为 TaskTableView
    TaskTableView *activeView = nullptr;

    if (uncompletedTableView->hasFocus() || uncompletedTableView->selectionModel()->hasSelection()) {
        activeView = uncompletedTableView;
    } else if (completedTableView->hasFocus() || completedTableView->selectionModel()->hasSelection()) {
        activeView = completedTableView;
    }

    if (!activeView || !activeView->selectionModel()->hasSelection()) {
        return -1;
    }

    QModelIndex proxyIndex = activeView->selectionModel()->selectedRows().first();

    QSortFilterProxyModel *proxy = qobject_cast<QSortFilterProxyModel*>(activeView->model());
    // 增加空指针检查
    if (!proxy) return -1;

    QModelIndex sourceIndex = proxy->mapToSource(proxyIndex);

    return taskModel->data(sourceIndex, TaskModel::IdRole).toInt();
}

void MainWindow::onQuickRecordClicked()
{
    InspirationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap data = dialog.getData();
        if (inspirationModel->addInspiration(data["content"].toString(), data["tags"].toString())) {
            updateStatusBar("灵感记录成功！");
            // 如果当前在灵感Tab，视图会自动刷新
        } else {
            QMessageBox::warning(this, "错误", "记录失败");
        }
    }
}

void MainWindow::onCalendarDateClicked(const QDate &date)
{
    // 查询该日期的灵感
    QList<QVariantMap> inspirations = inspirationModel->getInspirationsByDate(date);

    if (inspirations.isEmpty()) {
        return;
    }

    // 创建简单的查看对话框
    QDialog dlg(this);
    dlg.setWindowTitle(QString("灵感记录 - %1").arg(date.toString("MM月dd日")));
    dlg.resize(400, 500);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QListWidget *listWidget = new QListWidget(&dlg);
    listWidget->setAlternatingRowColors(true);
    listWidget->setStyleSheet("QListWidget { border: 1px solid #3d3d3d; background-color: #2d2d2d; } "
                              "QListWidget::item { padding: 10px; border-bottom: 1px solid #3d3d3d; }");

    for (const QVariantMap &data : inspirations) {
        QString timeStr = data["created_at"].toDateTime().toString("HH:mm");
        QString content = data["content"].toString();
        QString tags = data["tags"].toString();

        QString displayText = QString("[%1] %2").arg(timeStr, content);
        if (!tags.isEmpty()) {
            displayText += QString("\n标签: %1").arg(tags);
        }

        QListWidgetItem *item = new QListWidgetItem(displayText);
        listWidget->addItem(item);
    }

    layout->addWidget(listWidget);

    QPushButton *closeBtn = new QPushButton("关闭", &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
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

    // 使用资源路径加载样式表
    QStringList stylePaths = {
        ":/styles/mainwindow.qss",
        ":/styles/widget.qss",
        ":/styles/kanban.qss",
        ":/styles/calendar.qss",
        ":/styles/dialog.qss",

    };

    for (const QString &path : stylePaths) {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            styleSheet += QLatin1String(file.readAll());
            styleSheet += "\n";
            file.close();
        } else {
            qDebug() << "样式加载失败：" << path;
        }
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

// 灵感弹窗：长条形
void MainWindow::onCalendarShowInspirations(const QDate &date)
{
    QList<QVariantMap> inspirations = inspirationModel->getInspirationsByDate(date);
    if (inspirations.isEmpty()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QString("灵感 - %1").arg(date.toString("MM月dd日")));
    dlg.resize(350, 500); // 长条形
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QListWidget *listWidget = new QListWidget(&dlg);
    listWidget->setAlternatingRowColors(true);
    listWidget->setStyleSheet("QListWidget { border: none; background-color: #2d2d2d; } "
                              "QListWidget::item { padding: 10px; border-bottom: 1px solid #3d3d3d; }");

    for (const QVariantMap &data : inspirations) {
        QString timeStr = data["created_at"].toDateTime().toString("HH:mm");
        QString content = data["content"].toString();
        QString tags = data["tags"].toString();

        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QString("[%1] %2\n🏷️ %3").arg(timeStr, content, tags));
        listWidget->addItem(item);
    }
    layout->addWidget(listWidget);
    dlg.exec();
}

// 任务弹窗：扁平形
void MainWindow::onCalendarShowTasks(const QDate &date)
{
    // 获取当天任务 (需要 TaskModel 提供接口，或者遍历)
    // 这里简单遍历一下，实际建议在 TaskModel 加 getTasksByDate
    QList<QVariantMap> allTasks = taskModel->getAllTasks(false);
    QList<QVariantMap> dayTasks;
    for(const auto &t : allTasks) {
        if(t["deadline"].toDateTime().date() == date) {
            dayTasks.append(t);
        }
    }

    if (dayTasks.isEmpty()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QString("任务 - %1").arg(date.toString("MM月dd日")));
    dlg.resize(500, 300); // 扁平形
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QTableWidget *table = new QTableWidget(&dlg);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"ID", "标题", "截止时间"});
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->verticalHeader()->hide();
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setRowCount(dayTasks.size());

    for(int i=0; i<dayTasks.size(); ++i) {
        const auto &t = dayTasks[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::number(t["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(t["title"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(t["deadline"].toDateTime().toString("HH:mm")));
    }

    layout->addWidget(table);
    dlg.exec();
}
