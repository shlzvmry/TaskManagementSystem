#ifndef STATISTICVIEW_H
#define STATISTICVIEW_H

#include <QWidget>
#include <QDateTime>
#include "models/statisticmodel.h"
#include <QTextEdit>

class TaskModel;
class SimpleChartWidget;
class QLabel;
class QComboBox;
class QDateEdit;
class QListWidget;

class StatisticView : public QWidget
{
    Q_OBJECT
public:
    explicit StatisticView(QWidget *parent = nullptr);
    void setModels(TaskModel *taskModel, StatisticModel *statModel);
    void refresh();

private slots:
    void onFilterChanged();
    void onTimeRangeTypeChanged(int index);
    void onExportExcel();
    void onExportPDF();
    void requestDelayedRefresh(); // 防抖刷新（延迟刷新）

private:
    StatisticModel *m_statModel;
    TaskModel *m_taskModel;

    // 筛选控件
    QComboBox *m_timeRangeCombo;
    QWidget *m_customDateWidget;
    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QListWidget *m_categoryList;

    // 概览指标
    QLabel *m_totalLab, *m_compLab, *m_rateLab, *m_overdueLab, *m_avgTimeLab, *m_inspLab;

    // 图表
    SimpleChartWidget *m_catePie, *m_prioBar, *m_statusPie, *m_trendLine;

    // AI 分析框
    QTextEdit *m_aiAnalysisEdit;

    // 防抖定时器
    class QTimer *m_refreshTimer;

    void setupUI();
    StatisticModel::Filter getCurrentFilter() const;
    void updateContent();

};

#endif // STATISTICVIEW_H
