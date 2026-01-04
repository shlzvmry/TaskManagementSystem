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

StatisticView::StatisticView(QWidget *parent) : QWidget(parent)
{
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
    m_timeRangeCombo->addItems({"本周", "本日", "本月", "本年"});
    fLayout->addWidget(m_timeRangeCombo);

    // 常驻的日期选择器
    fLayout->addWidget(new QLabel("开始日期:"));
    m_startDateEdit = new QDateEdit(QDate::currentDate());
    m_startDateEdit->setCalendarPopup(true);
    fLayout->addWidget(m_startDateEdit);

    fLayout->addWidget(new QLabel("结束日期:"));
    m_endDateEdit = new QDateEdit(QDate::currentDate());
    m_endDateEdit->setCalendarPopup(true);
    fLayout->addWidget(m_endDateEdit);

    // 关键布局：在此处加弹簧，将上方的“预设/日期”和下方的“分类/按钮”拉开
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
    QHBoxLayout *row1 = new QHBoxLayout();
    m_catePie = new SimpleChartWidget(SimpleChartWidget::PieChart, "分类分布");
    m_trendLine = new SimpleChartWidget(SimpleChartWidget::LineChart, "完成动态趋势");
    row1->addWidget(m_catePie, 1);
    row1->addWidget(m_trendLine, 2);
    cLayout->addLayout(row1);

    QHBoxLayout *row2 = new QHBoxLayout();
    m_prioBar = new SimpleChartWidget(SimpleChartWidget::BarChart, "优先级分析");
    m_statusPie = new SimpleChartWidget(SimpleChartWidget::PieChart, "执行状态占比");
    row2->addWidget(m_prioBar, 1);
    row2->addWidget(m_statusPie, 1);
    cLayout->addLayout(row2);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnXls = new QPushButton(" 导出数据报告(CSV)");
    btnXls->setObjectName("applyFilterBtn");
    btnXls->setIcon(QIcon(":/icons/export_icon.png"));

    QPushButton *btnPdf = new QPushButton(" 生成统计周报(PDF)");
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

    // 关键：手动修改日期也触发更新（使用 userDateChanged 区分手动还是代码修改）
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

    QVariantMap ov = m_statModel->getOverviewStats(f);
    m_totalLab->setText(ov["total"].toString());
    m_compLab->setText(ov["completed"].toString());
    m_rateLab->setText(QString::number(ov["rate"].toDouble(), 'f', 1) + "%");
    m_overdueLab->setText(ov["overdue"].toString());
    m_avgTimeLab->setText(QString::number(m_statModel->getAverageCompletionTime(f), 'f', 1));
    m_inspLab->setText(QString::number(m_statModel->getInspirationCount(f)));

    m_catePie->setCategoryData(m_statModel->getTasksCountByCategory(f));
    m_prioBar->setCategoryData(m_statModel->getTasksCountByPriority(f));
    m_statusPie->setCategoryData(m_statModel->getTasksCountByStatus(f));
    m_trendLine->setTrendData(m_statModel->getDailyCompletionTrend(f));
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
    QString fileName = QFileDialog::getSaveFileName(this, "导出CSV", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    // 目前 exportTasksToCSV 内部还没适配 Filter，我们可以直接导出全部，
    // 或者后续再根据需要给 exportTasksToCSV 也增加 Filter 参数。
    if (Exporter::exportTasksToCSV(fileName, m_taskModel)) {
        QMessageBox::information(this, "成功", "任务列表已成功导出！");
    } else {
        QMessageBox::warning(this, "错误", "导出失败，请检查文件权限。");
    }
}
void StatisticView::onExportPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出PDF报表", "", "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    // 传入当前 UI 上的筛选条件
    if (Exporter::exportReportToPDF(fileName, m_taskModel, m_statModel, getCurrentFilter())) {
        QMessageBox::information(this, "成功", "PDF报表已生成！");
    } else {
        QMessageBox::warning(this, "错误", "生成PDF失败。");
    }
}
