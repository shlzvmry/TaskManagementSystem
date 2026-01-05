#include "mainwindow.h"
#include "database/database.h"
#include "widgets/watermarkwidget.h"
#include "models/taskmodel.h"
#include "models/inspirationmodel.h"
#include "dialogs/taskdialog.h"
#include "dialogs/recyclebindialog.h"
#include "dialogs/tagmanagerdialog.h"
#include "models/taskfiltermodel.h"
#include "views/kanbanview.h"
#include "views/calenderview.h"
#include "views/tasktableview.h"
#include "views/inspirationview.h"
#include "dialogs/inspirationdialog.h"
#include "views/statisticview.h"
#include "models/statisticmodel.h"
#include "threads/remindthread.h"
#include "dialogs/firstrundialog.h"
#include <QFileDialog>
#include <QGroupBox>
#include <QColorDialog>
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
#include <QProcess>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , taskModel(nullptr)
    , inspirationModel(nullptr)
    , taskTableView(nullptr)
    , recycleBinDialog(nullptr)
{
    Database::instance().initDatabase();

    loadStyleSheet();

    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int width = screenGeometry.width() * 3 / 4;
    int height = screenGeometry.height() * 3 / 4;
    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;

    setGeometry(x, y, width, height);
    setWindowTitle("个人工作与任务管理系统");

    taskModel = new TaskModel(this);
    inspirationModel = new InspirationModel(this);
    statisticModel = new StatisticModel(this);

    recycleBinDialog = new RecycleBinDialog(this);
    recycleBinDialog->setTaskModel(taskModel);
    // 首次运行检查
    if (Database::instance().getSetting("first_run", "true") == "true") {
        FirstRunDialog firstRunDlg(this);
        firstRunDlg.exec();
        // 刷新一下分类下拉框
        if (filterCategoryCombo) {
            filterCategoryCombo->clear();
            filterCategoryCombo->addItem("所有分类", -1);
            filterCategoryCombo->addItem("灵感记录✨", -2);
            QList<QVariantMap> cats = Database::instance().getAllCategories();
            for(const auto &cat : cats) {
                filterCategoryCombo->addItem(cat["name"].toString(), cat["id"]);
            }
        }
    }

    // 启动后台提醒线程
    remindThread = new RemindThread(this);
    connect(remindThread, &RemindThread::taskOverdueUpdated, this, [this](){
        // 逾期状态更新后刷新界面
        QMetaObject::invokeMethod(this, "onRefreshTasksClicked", Qt::QueuedConnection);
    });
    connect(remindThread, &RemindThread::remindTask, this, &MainWindow::onTaskReminded);
    remindThread->start();

    createWatermark();
    setupSystemTray();
    setupUI();
    setupConnections();

    if (recycleBinDialog && taskModel) {
        recycleBinDialog->setTaskModel(taskModel);
    }
}

MainWindow::~MainWindow()
{
    if (remindThread) {
        remindThread->stop();
        remindThread->wait();
        delete remindThread;
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 0);
    mainLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel("个人工作与任务管理系统", centralWidget);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    tabWidget = new QTabWidget(centralWidget);
    tabWidget->setObjectName("mainTabWidget");

    createTaskTab();
    createInspirationTab();
    createStatisticTab();
    createSettingTab();

    mainLayout->addWidget(tabWidget);

    statusBarWidget = new QStatusBar(this);
    setStatusBar(statusBarWidget);
    updateStatusBar(QString("就绪 | 任务总数: %1 | 已完成: %2 | 回收站: %3")
                        .arg(taskModel->getTaskCount())
                        .arg(taskModel->getCompletedCount())
                        .arg(taskModel->getDeletedTaskCount()));

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

    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(new QLabel("过滤:", taskTab));
    toolbarLayout->addSpacing(-20);
    filterCategoryCombo = new QComboBox(taskTab);
    filterCategoryCombo->setObjectName("filterCategoryCombo");
    filterCategoryCombo->addItem("所有分类", -1);
    filterCategoryCombo->addItem("灵感记录✨", -2);

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
    searchEdit->setFixedWidth(210);
    toolbarLayout->addSpacing(20);
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
    uncompletedTableView->setModel(uncompletedProxyModel);

    connect(uncompletedTableView, &TaskTableView::editTaskRequested,
            this, &MainWindow::onEditTask);

    QWidget *bottomContainer = new QWidget(taskSplitter);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomContainer);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    QPushButton *separatorBtn = new QPushButton(bottomContainer);
    separatorBtn->setObjectName("completedSeparatorBtn");
    separatorBtn->setCursor(Qt::PointingHandCursor);
    separatorBtn->setFixedHeight(7);
    separatorBtn->setFlat(true);

    completedTableView = new TaskTableView(bottomContainer);
    completedTableView->setObjectName("completedTableView");
    completedTableView->setModel(completedProxyModel);

    connect(completedTableView, &TaskTableView::editTaskRequested,
            this, &MainWindow::onEditTask);

    completedTableView->horizontalHeader()->hide();
    completedTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    completedTableView->setFrameShape(QFrame::NoFrame);

    connect(separatorBtn, &QPushButton::clicked, [this]() {
        bool isVisible = completedTableView->horizontalHeader()->isVisible();
        completedTableView->horizontalHeader()->setVisible(!isVisible);
    });

    bottomLayout->addWidget(separatorBtn);
    bottomLayout->addWidget(completedTableView);

    taskSplitter->addWidget(uncompletedTableView);
    taskSplitter->addWidget(bottomContainer);
    taskSplitter->setStretchFactor(0, 7);
    taskSplitter->setStretchFactor(1, 3);

    listLayout->addWidget(taskSplitter);

    kanbanView = new KanbanView(taskTab);
    kanbanView->setModel(taskModel);

    connect(kanbanView, &KanbanView::editTaskRequested, this, &MainWindow::onEditTask);

    calendarView = new CalendarView(taskTab);
    calendarView->setTaskModel(taskModel);
    calendarView->setInspirationModel(inspirationModel);
    viewStack->addWidget(listViewWidget);
    viewStack->addWidget(kanbanView);
    viewStack->addWidget(calendarView);

    connect(calendarView, &CalendarView::showInspirations, this, &MainWindow::onCalendarShowInspirations);
    connect(calendarView, &CalendarView::showTasks, this, &MainWindow::onCalendarShowTasks);

    QHBoxLayout *bottomBarLayout = new QHBoxLayout();

    QWidget *leftContainer = new QWidget(taskTab);
    leftContainer->setFixedWidth(110);
    QHBoxLayout *leftContainerLayout = new QHBoxLayout(leftContainer);
    leftContainerLayout->setContentsMargins(0, 0, 0, 0);

    kanbanGroupBtn = new QPushButton("分组: 状态", leftContainer);
    kanbanGroupBtn->setObjectName("kanbanGroupBtn");
    kanbanGroupBtn->setCursor(Qt::PointingHandCursor);
    kanbanGroupBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    kanbanGroupBtn->setVisible(false);

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

    QWidget *dummyRight = new QWidget(taskTab);
    dummyRight->setFixedWidth(110);

    bottomBarLayout->addWidget(leftContainer);
    bottomBarLayout->addStretch();
    bottomBarLayout->addLayout(centerBtnLayout);
    bottomBarLayout->addStretch();
    bottomBarLayout->addWidget(dummyRight);

    connect(viewGroup, &QButtonGroup::idClicked, this, [this](int id){
        viewStack->setCurrentIndex(id);
        if (kanbanGroupBtn) {
            kanbanGroupBtn->setVisible(id == 1);
        }
    });

    auto updateFilters = [this]() {
        int catId = filterCategoryCombo->currentData().toInt();
        int pri = filterPriorityCombo->currentData().toInt();
        QString text = searchEdit->text();

        if (catId == -2) {
            uncompletedProxyModel->setFilterCategory(-999);
            completedProxyModel->setFilterCategory(-999);
        } else {
            uncompletedProxyModel->setFilterCategory(catId);
            completedProxyModel->setFilterCategory(catId);
        }

        uncompletedProxyModel->setFilterPriority(pri);
        uncompletedProxyModel->setFilterText(text);

        completedProxyModel->setFilterPriority(pri);
        completedProxyModel->setFilterText(text);

        if (calendarView) {
            calendarView->setFilter(catId, pri);
        }
    };

    connect(filterCategoryCombo, &QComboBox::currentIndexChanged, this, updateFilters);
    connect(filterPriorityCombo, &QComboBox::currentIndexChanged, this, updateFilters);
    connect(searchEdit, &QLineEdit::textChanged, this, updateFilters);

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

    // 1. 添加任务
    QAction *addTaskAction = new QAction(QIcon(":/icons/add_icon.png"), "添加任务", this);
    connect(addTaskAction, &QAction::triggered, this, &MainWindow::onAddTaskClicked);
    trayMenu->addAction(addTaskAction);

    // 2. 记录灵感
    QAction *addInspirationAction = new QAction(QIcon(":/icons/edit_icon.png"), "记录灵感", this);
    connect(addInspirationAction, &QAction::triggered, this, &MainWindow::onQuickRecordClicked);
    trayMenu->addAction(addInspirationAction);

    trayMenu->addSeparator();

    QAction *showAction = new QAction("显示主窗口", this);
    connect(showAction, &QAction::triggered, this, &MainWindow::showMainWindow);
    trayMenu->addAction(showAction);

    trayMenu->addSeparator();

    QAction *quitAction = new QAction("退出", this);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    trayMenu->addAction(quitAction);

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
    layout->setContentsMargins(0, 0, 0, 0);

    statisticView = new StatisticView(statisticTab);
    statisticView->setModels(taskModel, statisticModel);

    layout->addWidget(statisticView);

    tabWidget->addTab(statisticTab, "统计分析");

    // 切换到统计标签页时刷新数据
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (tabWidget->tabText(index) == "统计分析") {
            statisticView->refresh();
        }
    });
}

void MainWindow::createSettingTab()
{
    QWidget *settingTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(settingTab);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 数据管理区域
    QGroupBox *dataGroup = new QGroupBox("💾 数据安全", settingTab);
    QHBoxLayout *dataLayout = new QHBoxLayout(dataGroup);

    QPushButton *backupBtn = new QPushButton("备份数据库", dataGroup);
    backupBtn->setIcon(QIcon(":/icons/export_icon.png"));
    connect(backupBtn, &QPushButton::clicked, this, &MainWindow::onBackupDatabase);

    QPushButton *restoreBtn = new QPushButton("恢复数据库", dataGroup);
    restoreBtn->setIcon(QIcon(":/icons/refresh_icon.png"));
    connect(restoreBtn, &QPushButton::clicked, this, &MainWindow::onRestoreDatabase);

    dataLayout->addWidget(backupBtn);
    dataLayout->addWidget(restoreBtn);
    dataLayout->addStretch();

    // 2. 分类管理区域
    QGroupBox *catGroup = new QGroupBox("🗂️ 任务分类管理", settingTab);
    QVBoxLayout *catLayout = new QVBoxLayout(catGroup);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    settingCategoryEdit = new QLineEdit(catGroup);
    settingCategoryEdit->setPlaceholderText("输入新分类名称...");

    QPushButton *addCatBtn = new QPushButton("添加分类", catGroup);
    connect(addCatBtn, &QPushButton::clicked, this, &MainWindow::onAddCategory);

    inputLayout->addWidget(settingCategoryEdit);
    inputLayout->addWidget(addCatBtn);

    settingCategoryList = new QListWidget(catGroup);
    settingCategoryList->setAlternatingRowColors(true);

    // 刷新列表
    auto refreshCatList = [this]() {
        settingCategoryList->clear();
        QList<QVariantMap> cats = Database::instance().getAllCategories();
        for(const auto &c : cats) {
            QListWidgetItem *item = new QListWidgetItem(c["name"].toString());
            item->setData(Qt::UserRole, c["id"]);
            QPixmap pix(16,16);
            pix.fill(QColor(c["color"].toString()));
            item->setIcon(QIcon(pix));
            settingCategoryList->addItem(item);
        }
    };
    refreshCatList();

    // 切换到设置页时自动刷新分类列表
    connect(tabWidget, &QTabWidget::currentChanged, this, [this, refreshCatList](int index){
        if(tabWidget->tabText(index) == "系统设置") refreshCatList();
    });

    QPushButton *delCatBtn = new QPushButton("删除选中分类", catGroup);
    delCatBtn->setStyleSheet("background-color: #C96A6A; color: white; border: none; padding: 5px; border-radius: 4px;");
    connect(delCatBtn, &QPushButton::clicked, this, &MainWindow::onDeleteCategory);

    catLayout->addLayout(inputLayout);
    catLayout->addWidget(settingCategoryList);
    catLayout->addWidget(delCatBtn);

    // 3. 偏好设置
    QGroupBox *prefGroup = new QGroupBox("⚙️ 偏好设置", settingTab);
    QVBoxLayout *prefLayout = new QVBoxLayout(prefGroup);
    QCheckBox *soundCheck = new QCheckBox("启用提醒音效", prefGroup);
    soundCheck->setChecked(Database::instance().getSetting("sound_enabled", "true") == "true");
    connect(soundCheck, &QCheckBox::toggled, [](bool checked){
        Database::instance().setSetting("sound_enabled", checked ? "true" : "false");
    });

    prefLayout->addWidget(soundCheck);

    mainLayout->addWidget(dataGroup);
    mainLayout->addWidget(catGroup);
    mainLayout->addWidget(prefGroup);
    mainLayout->addStretch();

    tabWidget->addTab(settingTab, "系统设置");
}


void MainWindow::setupConnections()
{
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

    if (taskTableView) {
        connect(taskTableView, &QTableView::doubleClicked, this, &MainWindow::onTaskDoubleClicked);
    }

    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this, SLOT(onAddTaskClicked()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this, SLOT(onEditTaskClicked()));
    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteTaskClicked()));
    new QShortcut(QKeySequence(Qt::Key_F5), this, SLOT(onRefreshTasksClicked()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I), this, SLOT(onQuickRecordClicked()));

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

    // --- 自动检查逾期任务逻辑 ---
    overdueCheckTimer = new QTimer(this);
    connect(overdueCheckTimer, &QTimer::timeout, this, [this]() {
        if (taskModel) {
            taskModel->checkOverdueTasks();
        }
    });
    overdueCheckTimer->start(60000); // 每60秒检查一次

    // 启动后延迟1秒执行一次初次检查
    QTimer::singleShot(1000, this, [this](){
        if (taskModel) taskModel->checkOverdueTasks();
    });
}

void MainWindow::onRecycleBinClicked()
{
    if (recycleBinDialog) {
        recycleBinDialog->refreshDeletedTasks();
        recycleBinDialog->exec();
    }
}
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
        } else {
            QMessageBox::warning(this, "错误", "记录失败");
        }
    }
}

void MainWindow::onCalendarDateClicked(const QDate &date)
{
    QList<QVariantMap> inspirations = inspirationModel->getInspirationsByDate(date);

    if (inspirations.isEmpty()) {
        return;
    }

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

    QStringList stylePaths = {
        ":/styles/mainwindow.qss",
        ":/styles/widget.qss",
        ":/styles/kanban.qss",
        ":/styles/calendar.qss",
        ":/styles/dialog.qss",
        ":/styles/statistic.qss",

    };

    for (const QString &path : stylePaths) {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            styleSheet += QString::fromUtf8(file.readAll());
            styleSheet += "\n";
            file.close();
        } else {
            qDebug() << "样式加载失败：" << path;
        }
    }

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

void MainWindow::onCalendarShowInspirations(const QDate &date)
{
    QList<QVariantMap> inspirations = inspirationModel->getInspirationsByDate(date);
    if (inspirations.isEmpty()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QString("灵感 - %1").arg(date.toString("MM月dd日")));
    dlg.resize(350, 500);
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

void MainWindow::onCalendarShowTasks(const QDate &date)
{
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
    dlg.resize(500, 300);
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

void MainWindow::onBackupDatabase()
{
    QString fileName = QFileDialog::getSaveFileName(this, "备份数据库",
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/task_backup.db",
                                                    "Database Files (*.db)");
    if (fileName.isEmpty()) return;

    if (Database::instance().backupDatabase(fileName)) {
        QMessageBox::information(this, "成功", "数据库备份成功！");
    } else {
        QMessageBox::warning(this, "失败", "备份失败，请检查文件权限。");
    }
}

void MainWindow::onRestoreDatabase()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择备份文件",
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                    "Database Files (*.db)");
    if (fileName.isEmpty()) return;

    if (QMessageBox::warning(this, "警告", "恢复操作将覆盖当前所有数据且不可撤销！\n确定要继续吗？",
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        // 停止线程防止占用数据库
        if(remindThread) remindThread->stop();

        if (Database::instance().restoreDatabase(fileName)) {
            QMessageBox::information(this, "成功", "数据恢复成功！程序将重启以应用更改。");
            qApp->quit();
            QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
        } else {
            QMessageBox::warning(this, "失败", "恢复失败，可能是文件损坏或被占用。");
            // 重启线程
            if(remindThread) remindThread->start();
        }
    }
}

void MainWindow::onAddCategory()
{
    QString name = settingCategoryEdit->text().trimmed();
    if (name.isEmpty()) return;

    // 随机颜色
    QStringList colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7", "#DDA0DD", "#7696B3"};
    QString color = colors[rand() % colors.size()];

    if (Database::instance().addCategory(name, color)) {
        settingCategoryEdit->clear();
        QMessageBox::information(this, "成功", "分类添加成功");
    } else {
        QMessageBox::warning(this, "错误", "分类已存在或添加失败");
    }
}

void MainWindow::onDeleteCategory()
{
    QListWidgetItem *item = settingCategoryList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "确认", "删除分类将导致该分类下的任务变为'未分类'，确定删除吗？") == QMessageBox::Yes) {
        Database::instance().deleteCategory(id);
        delete item;
    }
}

void MainWindow::onTaskReminded(int taskId, const QString &title)
{
    Q_UNUSED(taskId);
    // 播放系统提示音
    if (Database::instance().getSetting("sound_enabled", "true") == "true") {
        QApplication::beep();
    }

    // 显示托盘消息
    if (trayIcon) {
        trayIcon->showMessage("任务提醒", QString("任务即将到期：\n%1").arg(title), QSystemTrayIcon::Information, 5000);
    }
}
