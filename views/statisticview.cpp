#include "statisticview.h"
#include "models/taskmodel.h"
#include "widgets/simplechartwidget.h"
#include "utils/exporter.h"
#include "database/database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QDateEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

StatisticView::StatisticView(QWidget *parent) : QWidget(parent)
{
    // 初始化防抖定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(300); // 300ms 延迟
    connect(m_refreshTimer, &QTimer::timeout, this, &StatisticView::onFilterChanged);

    setupUI();
}
void StatisticView::setupUI()
{
    this->setObjectName("statisticView");
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    // --- 1. 左侧筛选栏 ---
    QWidget *filterSide = new QWidget();
    filterSide->setObjectName("statisticFilterSide");
    filterSide->setFixedWidth(220);
    QVBoxLayout *fLayout = new QVBoxLayout(filterSide);
    fLayout->setContentsMargins(15, 20, 15, 20);
    fLayout->setSpacing(8);

    // 时间预设下拉框
    QLabel *timeLabel = new QLabel("时间范围预设:");
    timeLabel->setObjectName("filterLabel");
    fLayout->addWidget(timeLabel);

    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->setObjectName("filterCategoryCombo");
    m_timeRangeCombo->addItems({"本周", "本日", "本月", "本年", "自定义"}); // 修改：增加自定义选项
    fLayout->addWidget(m_timeRangeCombo);

    // 常驻的日期选择器
    fLayout->addWidget(new QLabel("开始日期:"));
    m_startDateEdit = new QDateEdit(QDate::currentDate());
    m_startDateEdit->setCalendarPopup(true);
    fLayout->addWidget(m_startDateEdit);

    fLayout->addWidget(new QLabel("结束日期:"));
    m_endDateEdit = new QDateEdit(QDate::currentDate());
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setMaximumDate(QDate::currentDate());
    fLayout->addWidget(m_endDateEdit);

    // 日期联动：开始日期改变时，限制结束日期的最小值
    connect(m_startDateEdit, &QDateEdit::dateChanged, this, [this](QDate date){
        m_endDateEdit->setMinimumDate(date);
        if (m_endDateEdit->date() < date) {
            m_endDateEdit->setDate(date);
        }
    });
    m_endDateEdit->setMinimumDate(m_startDateEdit->date());

    auto switchToCustom = [this]() {
        // 阻断信号防止递归调用 onTimeRangeTypeChanged
        bool oldState = m_timeRangeCombo->blockSignals(true);
        m_timeRangeCombo->setCurrentIndex(4); // 4 是 "自定义" 的索引
        m_timeRangeCombo->blockSignals(oldState);

        // 使用防抖刷新，防止滚轮快速滚动导致卡死
        requestDelayedRefresh();
    };

    // 使用 userDateChanged 仅响应用户手动操作
    connect(m_startDateEdit, &QDateEdit::userDateChanged, this, switchToCustom);
    connect(m_endDateEdit, &QDateEdit::userDateChanged, this, switchToCustom);

    fLayout->addStretch(1);

    // 底部区域：分类和按钮
    QLabel *cateLabel = new QLabel("任务分类 (多选):");
    cateLabel->setObjectName("filterLabel");
    fLayout->addWidget(cateLabel);

    m_categoryList = new QListWidget();
    m_categoryList->setObjectName("categoryListWidget");
    m_categoryList->setSpacing(2);
    m_categoryList->setFixedHeight(220); // 调整高度使整体更紧凑
    m_categoryList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QList<QVariantMap> cats = Database::instance().getAllCategories();
    for (const auto &c : cats) {
        QListWidgetItem *item = new QListWidgetItem(c["name"].toString(), m_categoryList);
        item->setData(Qt::UserRole, c["id"]);
        item->setCheckState(Qt::Checked);
        item->setSizeHint(QSize(0, 32));
    }
    fLayout->addWidget(m_categoryList);

    QPushButton *applyBtn = new QPushButton("更新分析");
    applyBtn->setObjectName("applyFilterBtn");
    applyBtn->setCursor(Qt::PointingHandCursor);
    fLayout->addWidget(applyBtn);

    // --- 2. 右侧滚动区域 ---
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->verticalScrollBar()->setSingleStep(20);

    QWidget *content = new QWidget();
    content->setObjectName("statisticContentWidget");
    QVBoxLayout *cLayout = new QVBoxLayout(content);
    cLayout->setContentsMargins(25, 25, 25, 25);
    cLayout->setSpacing(30);

    // 指标卡片
    QHBoxLayout *cardLayout = new QHBoxLayout();
    auto makeCard = [&](const QString &t, QLabel*& l, bool isOverdue = false) {
        QFrame *f = new QFrame();
        f->setProperty("class", "statCard");
        QVBoxLayout *vl = new QVBoxLayout(f);
        QLabel *title = new QLabel(t);
        title->setObjectName("statTitleLabel");
        l = new QLabel("0");
        l->setObjectName(isOverdue ? "overdueValueLabel" : "statValueLabel");
        vl->addWidget(title); vl->addWidget(l, 0, Qt::AlignCenter);
        cardLayout->addWidget(f);
    };
    makeCard("总任务", m_totalLab);
    makeCard("已完成", m_compLab);
    makeCard("完成率", m_rateLab);
    makeCard("已逾期", m_overdueLab, true);
    makeCard("平均耗时(h)", m_avgTimeLab);
    makeCard("产生灵感", m_inspLab);
    cLayout->addLayout(cardLayout);

    // 图表绘制部分
    // 第一行：分类分布 + AI分析建议
    QHBoxLayout *row1 = new QHBoxLayout();
    m_catePie = new SimpleChartWidget(SimpleChartWidget::PieChart, "分类分布");

    // 新增：AI 分析框
    QFrame *aiFrame = new QFrame();
    aiFrame->setObjectName("aiAnalysisFrame");
    aiFrame->setStyleSheet("QFrame#aiAnalysisFrame { background-color: #2d2d2d; border: 1px solid #3d3d3d; border-radius: 10px; }");
    QVBoxLayout *aiLayout = new QVBoxLayout(aiFrame);

    QLabel *aiTitle = new QLabel("✨ AI 智能分析与建议");
    aiTitle->setStyleSheet("background: transparent; color: #657896; font-weight: bold; font-size: 11pt; padding: 5px;");

    m_aiAnalysisEdit = new QTextEdit();
    m_aiAnalysisEdit->setReadOnly(true);
    m_aiAnalysisEdit->setPlaceholderText("正在分析您的工作习惯...\n(此处预留对接 AI 模型 API，可根据图表数据自动生成周报总结与改进建议)");
    m_aiAnalysisEdit->setStyleSheet("border: none; background: transparent; color: #cccccc; font-size: 10pt;");

    aiLayout->addWidget(aiTitle);
    aiLayout->addWidget(m_aiAnalysisEdit);

    row1->addWidget(m_catePie,2);
    row1->addWidget(aiFrame, 3); // AI 框与饼图 2:3
    cLayout->addLayout(row1);

    // 第二行：完成动态趋势
    QHBoxLayout *row2 = new QHBoxLayout();
    m_trendLine = new SimpleChartWidget(SimpleChartWidget::LineChart, "完成动态趋势");
    // 设置最小高度，保证独占一行时美观
    m_trendLine->setMinimumHeight(300);
    row2->addWidget(m_trendLine);
    cLayout->addLayout(row2);

    // 第三行：优先级 + 状态
    QHBoxLayout *row3 = new QHBoxLayout();
    m_prioBar = new SimpleChartWidget(SimpleChartWidget::BarChart, "优先级分析");
    m_statusPie = new SimpleChartWidget(SimpleChartWidget::PieChart, "执行状态占比");
    row3->addWidget(m_prioBar, 1);
    row3->addWidget(m_statusPie, 1);
    cLayout->addLayout(row3);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnXls = new QPushButton(" 导出数据报告(CSV)");
    btnXls->setObjectName("applyFilterBtn");
    btnXls->setIcon(QIcon(":/icons/export_icon.png"));
    btnLayout->addSpacing(
        0);
    QPushButton *btnPdf = new QPushButton(" 生成统计报表(PDF)");
    btnPdf->setObjectName("applyFilterBtn");
    btnPdf->setIcon(QIcon(":/icons/export_icon.png"));

    btnLayout->addStretch();
    btnLayout->addWidget(btnXls);
    btnLayout->addSpacing(10);
    btnLayout->addWidget(btnPdf);
    cLayout->addLayout(btnLayout);

    scroll->setWidget(content);
    mainLayout->addWidget(filterSide);
    mainLayout->addWidget(scroll);

    // 信号绑定
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticView::onTimeRangeTypeChanged);

    // 关键：手动修改日期也触发更新
    connect(m_startDateEdit, &QDateEdit::userDateChanged, this, &StatisticView::onFilterChanged);
    connect(m_endDateEdit, &QDateEdit::userDateChanged, this, &StatisticView::onFilterChanged);

    connect(applyBtn, &QPushButton::clicked, this, &StatisticView::onFilterChanged);
    connect(btnXls, &QPushButton::clicked, this, &StatisticView::onExportExcel);
    connect(btnPdf, &QPushButton::clicked, this, &StatisticView::onExportPDF);
    connect(m_categoryList, &QListWidget::itemClicked, this, [](QListWidgetItem* item){
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    });

    // 初始化日期：触发一次“本周”的选择
    onTimeRangeTypeChanged(0);
}

StatisticModel::Filter StatisticView::getCurrentFilter() const
{
    StatisticModel::Filter f;

    // 直接从日期控件读取范围，不再关心下拉框选了什么
    f.start.setDate(m_startDateEdit->date());
    f.start.setTime(QTime(0, 0, 0));

    f.end.setDate(m_endDateEdit->date());
    f.end.setTime(QTime(23, 59, 59));

    for (int i = 0; i < m_categoryList->count(); ++i) {
        if (m_categoryList->item(i)->checkState() == Qt::Checked)
            f.categoryIds << m_categoryList->item(i)->data(Qt::UserRole).toInt();
    }
    return f;
}

void StatisticView::onTimeRangeTypeChanged(int index)
{
    if (index == 4) return; // 自定义模式：不自动修改日期，直接返回

    QDate now = QDate::currentDate();
    QDate start, end;

    switch (index) {
    case 0: // 本周
        start = now.addDays(-(now.dayOfWeek() - 1));
        end = now.addDays(7 - now.dayOfWeek());
        break;
    case 1: // 本日
        start = now;
        end = now;
        break;
    case 2: // 本月
        start = QDate(now.year(), now.month(), 1);
        end = QDate(now.year(), now.month(), now.daysInMonth());
        break;
    case 3: // 本年
        start = QDate(now.year(), 1, 1);
        end = QDate(now.year(), 12, 31);
        break;
    }

    // 更新日期选择器的值
    m_startDateEdit->setDate(start);
    m_endDateEdit->setDate(end);

    // 自动触发一次数据更新
    onFilterChanged();
}

void StatisticView::onFilterChanged()
{
    if (!m_statModel) return;
    updateContent();
}

void StatisticView::updateContent()
{
    StatisticModel::Filter f = getCurrentFilter();

    // 更新概览卡片... (保持原有代码不变)
    if (m_statModel) {
        QVariantMap stats = m_statModel->getOverviewStats(f);
        m_totalLab->setText(stats["total"].toString());
        m_compLab->setText(stats["completed"].toString());
        m_rateLab->setText(QString::number(stats["rate"].toDouble(), 'f', 1) + "%");
        m_overdueLab->setText(stats["overdue"].toString());
        double avgTime = m_statModel->getAverageCompletionTime(f);
        m_avgTimeLab->setText(QString::number(avgTime, 'f', 1));
        int inspCount = m_statModel->getInspirationCount(f);
        m_inspLab->setText(QString::number(inspCount));
    }

    // --- 趋势图逻辑修改 ---
    QVector<int> trendData;
    QStringList labels;
    QStringList tooltips;
    QString subTitle;

    int typeIndex = m_timeRangeCombo->currentIndex(); // 0:本周, 1:本日, 2:本月, 3:本年

    // 判断是否是自定义时间（如果日期被手动修改过，可能不对应下拉框）
    // 这里简单处理：如果下拉框选的是本年，就走本年逻辑；否则走天逻辑

    if (typeIndex == 3) {
        // 规则 3.3: 选择“本年”时横坐标只需要12个点，对应每个月的m1、m2...
        trendData = m_statModel->getMonthlyTrend(f);
        for(int i=1; i<=12; ++i) {
            labels << QString("m%1").arg(i);
            tooltips << QString("%1年%2月").arg(f.start.date().year()).arg(i);
        }
        subTitle = QString("%1年").arg(f.start.date().year());
    }
    else {
        // 按天统计 (本日、本周、本月、自定义)
        QDate startDate = f.start.date();
        QDate endDate = f.end.date();
        QDate today = QDate::currentDate();

        // 规则 3.1: 把具体的日期显示在左上角
        subTitle = QString("%1 ~ %2").arg(startDate.toString("yyyy.MM.dd")).arg(endDate.toString("yyyy.MM.dd"));

        // 规则 3.2: 当选择“本月”的时间范围时，未经过的天不画
        // 如果是本月模式 (typeIndex == 2)，结束日期截断到今天
        if (typeIndex == 2 && endDate > today) {
            endDate = today;
            // 重新设置Filter的end用于查询数据，但不改变界面上的日期选择器
            StatisticModel::Filter f_temp = f;
            f_temp.end.setDate(endDate);
            f_temp.end.setTime(QTime(23, 59, 59));
            trendData = m_statModel->getDailyTrend(f_temp);
        } else {
            trendData = m_statModel->getDailyTrend(f);
        }

        int days = startDate.daysTo(endDate) + 1;

        // 确保数据点数量匹配
        if (trendData.size() > days) trendData.resize(days);

        for(int i=0; i<days; ++i) {
            QDate d = startDate.addDays(i);
            // 规则 3.2: 横坐标显示 d1, d2...
            // 规则 3.3: 只有天数 <= 31 才显示横坐标文字 (ChartWidget内部控制)，这里只管传值
            labels << QString("d%1").arg(i + 1);
            // 鼠标悬停时显示具体日期
            tooltips << d.toString("yyyy-MM-dd");
        }
    }

    m_trendLine->setSubTitle(subTitle);
    m_trendLine->setTrendData(trendData, labels, tooltips);
    // ----------------------

    m_catePie->setCategoryData(m_statModel->getTasksCountByCategory(f));
    m_prioBar->setCategoryData(m_statModel->getTasksCountByPriority(f));
    m_statusPie->setCategoryData(m_statModel->getTasksCountByStatus(f));
}

void StatisticView::setModels(TaskModel *taskModel, StatisticModel *statModel)
{
    m_taskModel = taskModel;
    m_statModel = statModel;
    onFilterChanged();
}

void StatisticView::refresh() { onFilterChanged(); }

void StatisticView::onExportExcel()
{
    // 修改：使用系统“文档”目录，兼容性最好
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString exportPath = docPath + "/TaskManagement_Exports"; // 专用子文件夹
    QDir dir(exportPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString defaultFileName = exportPath + "/任务数据_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm") + ".csv";

    QString fileName = QFileDialog::getSaveFileName(this, "导出任务数据(CSV)", defaultFileName, "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    if (Exporter::exportTasksToCSV(fileName, m_taskModel, getCurrentFilter())) {
        QMessageBox::information(this, "成功", "导出成功！\n文件位置：" + fileName);
        // 可选：导出后自动打开文件夹，方便用户找到
        // QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(fileName).absolutePath()));
    } else {
        QMessageBox::warning(this, "错误", "导出失败，请检查文件权限。");
    }
}

void StatisticView::onExportPDF()
{
    // 修改：使用系统“文档”目录
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString exportPath = docPath + "/TaskManagement_Exports";
    QDir dir(exportPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString defaultFileName = exportPath + "/统计报表_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm") + ".pdf";

    QString fileName = QFileDialog::getSaveFileName(this, "导出统计报表(PDF)", defaultFileName, "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    QList<QPixmap> pixmaps;
    if (m_catePie) pixmaps << m_catePie->grab();
    if (m_trendLine) pixmaps << m_trendLine->grab();
    if (m_prioBar) pixmaps << m_prioBar->grab();
    if (m_statusPie) pixmaps << m_statusPie->grab();

    // 获取 AI 分析框的纯文本
    QString aiText = m_aiAnalysisEdit ? m_aiAnalysisEdit->toPlainText() : "";

    if (Exporter::exportReportToPDF(fileName, m_taskModel, m_statModel, getCurrentFilter(), pixmaps, aiText)) {
        QMessageBox::information(this, "成功", "报表生成成功！\n文件位置：" + fileName);
    } else {
        QMessageBox::warning(this, "错误", "生成PDF失败。");
    }
}

void StatisticView::requestDelayedRefresh()
{
    // 每次调用都会重置定时器，只有停止操作 300ms 后才会触发 timeout
    m_refreshTimer->start();
}
