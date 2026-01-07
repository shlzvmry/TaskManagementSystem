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
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(300);
    connect(m_refreshTimer, &QTimer::timeout, this, &StatisticView::onFilterChanged);

    setupUI();
}
void StatisticView::setupUI()
{
    this->setObjectName("statisticView");
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    QWidget *filterSide = new QWidget();
    filterSide->setObjectName("statisticFilterSide");
    filterSide->setFixedWidth(220);
    QVBoxLayout *fLayout = new QVBoxLayout(filterSide);
    fLayout->setContentsMargins(15, 20, 15, 20);
    fLayout->setSpacing(8);

    QLabel *timeLabel = new QLabel("时间范围预设:");
    timeLabel->setObjectName("filterLabel");
    fLayout->addWidget(timeLabel);

    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->setObjectName("filterCategoryCombo");
    m_timeRangeCombo->addItems({"本周", "本日", "本月", "本年", "自定义"}); // 修改：增加自定义选项
    fLayout->addWidget(m_timeRangeCombo);

    fLayout->addWidget(new QLabel("开始日期:"));
    m_startDateEdit = new QDateEdit(QDate::currentDate());
    m_startDateEdit->setCalendarPopup(true);
    fLayout->addWidget(m_startDateEdit);

    fLayout->addWidget(new QLabel("结束日期:"));
    m_endDateEdit = new QDateEdit(QDate::currentDate());
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setMaximumDate(QDate::currentDate());
    fLayout->addWidget(m_endDateEdit);

    connect(m_startDateEdit, &QDateEdit::dateChanged, this, [this](QDate date){
        m_endDateEdit->setMinimumDate(date);
        if (m_endDateEdit->date() < date) {
            m_endDateEdit->setDate(date);
        }
    });
    m_endDateEdit->setMinimumDate(m_startDateEdit->date());

    auto switchToCustom = [this]() {
        bool oldState = m_timeRangeCombo->blockSignals(true);
        m_timeRangeCombo->setCurrentIndex(4);
        m_timeRangeCombo->blockSignals(oldState);

        requestDelayedRefresh();
    };

    connect(m_startDateEdit, &QDateEdit::userDateChanged, this, switchToCustom);
    connect(m_endDateEdit, &QDateEdit::userDateChanged, this, switchToCustom);

    fLayout->addStretch(1);

    QLabel *cateLabel = new QLabel("任务分类 (多选):");
    cateLabel->setObjectName("filterLabel");
    fLayout->addWidget(cateLabel);

    m_categoryList = new QListWidget();
    m_categoryList->setObjectName("categoryListWidget");
    m_categoryList->setSpacing(2);
    m_categoryList->setFixedHeight(220);
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

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->verticalScrollBar()->setSingleStep(20);

    QWidget *content = new QWidget();
    content->setObjectName("statisticContentWidget");
    QVBoxLayout *cLayout = new QVBoxLayout(content);
    cLayout->setContentsMargins(25, 25, 25, 25);
    cLayout->setSpacing(30);

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

    QHBoxLayout *row1 = new QHBoxLayout();
    m_catePie = new SimpleChartWidget(SimpleChartWidget::PieChart, "分类分布");

    QFrame *aiFrame = new QFrame();
    aiFrame->setObjectName("aiAnalysisFrame");
    QVBoxLayout *aiLayout = new QVBoxLayout(aiFrame);

    QLabel *aiTitle = new QLabel("✨ AI 智能分析与建议");
    aiTitle->setObjectName("aiTitleLabel");

    m_aiAnalysisEdit = new QTextEdit();
    m_aiAnalysisEdit->setReadOnly(true);
    m_aiAnalysisEdit->setPlaceholderText("正在分析您的工作习惯...\n(此处预留对接 AI 模型 API，可根据图表数据自动生成周报总结与改进建议)");
    m_aiAnalysisEdit->setObjectName("aiContentEdit");

    aiLayout->addWidget(aiTitle);
    aiLayout->addWidget(m_aiAnalysisEdit);

    row1->addWidget(m_catePie,2);
    row1->addWidget(aiFrame, 3);
    cLayout->addLayout(row1);

    QHBoxLayout *row2 = new QHBoxLayout();
    m_trendLine = new SimpleChartWidget(SimpleChartWidget::LineChart, "完成动态趋势");
    m_trendLine->setMinimumHeight(300);
    row2->addWidget(m_trendLine);
    cLayout->addLayout(row2);

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

    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticView::onTimeRangeTypeChanged);

    connect(m_startDateEdit, &QDateEdit::userDateChanged, this, &StatisticView::onFilterChanged);
    connect(m_endDateEdit, &QDateEdit::userDateChanged, this, &StatisticView::onFilterChanged);

    connect(applyBtn, &QPushButton::clicked, this, &StatisticView::onFilterChanged);
    connect(btnXls, &QPushButton::clicked, this, &StatisticView::onExportExcel);
    connect(btnPdf, &QPushButton::clicked, this, &StatisticView::onExportPDF);
    connect(m_categoryList, &QListWidget::itemClicked, this, [](QListWidgetItem* item){
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    });

    onTimeRangeTypeChanged(0);
}

StatisticModel::Filter StatisticView::getCurrentFilter() const
{
    StatisticModel::Filter f;

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
    if (index == 4) return;

    QDate now = QDate::currentDate();
    QDate start, end;

    switch (index) {
    case 0:
        start = now.addDays(-(now.dayOfWeek() - 1));
        end = now.addDays(7 - now.dayOfWeek());
        break;
    case 1:
        start = now;
        end = now;
        break;
    case 2:
        start = QDate(now.year(), now.month(), 1);
        end = QDate(now.year(), now.month(), now.daysInMonth());
        break;
    case 3:
        start = QDate(now.year(), 1, 1);
        end = QDate(now.year(), 12, 31);
        break;
    }

    m_startDateEdit->setDate(start);
    m_endDateEdit->setDate(end);

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

    QVector<int> trendData;
    QStringList labels;
    QStringList tooltips;
    QString subTitle;

    int typeIndex = m_timeRangeCombo->currentIndex();

    if (typeIndex == 3) {
        trendData = m_statModel->getMonthlyTrend(f);
        for(int i=1; i<=12; ++i) {
            labels << QString("m%1").arg(i);
            tooltips << QString("%1年%2月").arg(f.start.date().year()).arg(i);
        }
        subTitle = QString("%1年").arg(f.start.date().year());
    }
    else {
        QDate startDate = f.start.date();
        QDate endDate = f.end.date();
        QDate today = QDate::currentDate();

        subTitle = QString("%1 ~ %2").arg(startDate.toString("yyyy.MM.dd")).arg(endDate.toString("yyyy.MM.dd"));

        if (typeIndex == 2 && endDate > today) {
            endDate = today;
            StatisticModel::Filter f_temp = f;
            f_temp.end.setDate(endDate);
            f_temp.end.setTime(QTime(23, 59, 59));
            trendData = m_statModel->getDailyTrend(f_temp);
        } else {
            trendData = m_statModel->getDailyTrend(f);
        }

        int days = startDate.daysTo(endDate) + 1;

        if (trendData.size() > days) trendData.resize(days);

        for(int i=0; i<days; ++i) {
            QDate d = startDate.addDays(i);
            labels << QString("d%1").arg(i + 1);
            tooltips << d.toString("yyyy-MM-dd");
        }
    }

    m_trendLine->setSubTitle(subTitle);
    m_trendLine->setTrendData(trendData, labels, tooltips);

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
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString exportPath = docPath + "/TaskManagement_Exports";
    QDir dir(exportPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString defaultFileName = exportPath + "/任务数据_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm") + ".csv";

    QString fileName = QFileDialog::getSaveFileName(this, "导出任务数据(CSV)", defaultFileName, "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    if (Exporter::exportTasksToCSV(fileName, m_taskModel, getCurrentFilter())) {
        QMessageBox::information(this, "成功", "导出成功！\n文件位置：" + fileName);
    } else {
        QMessageBox::warning(this, "错误", "导出失败，请检查文件权限。");
    }
}

void StatisticView::onExportPDF()
{
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

    QString aiText = m_aiAnalysisEdit ? m_aiAnalysisEdit->toPlainText() : "";

    if (Exporter::exportReportToPDF(fileName, m_taskModel, m_statModel, getCurrentFilter(), pixmaps, aiText)) {
        QMessageBox::information(this, "成功", "报表生成成功！\n文件位置：" + fileName);
    } else {
        QMessageBox::warning(this, "错误", "生成PDF失败。");
    }
}

void StatisticView::requestDelayedRefresh()
{
    m_refreshTimer->start();
}
