#include "calenderview.h"
#include "models/taskmodel.h"
#include <QPainter>
#include <QTextCharFormat>
#include <QDebug>

CalendarView::CalendarView(QWidget *parent)
    : QCalendarWidget(parent), m_model(nullptr)
{
    setGridVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(QColor("#99B6E3"));
    setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    setWeekdayTextFormat(Qt::Sunday, weekendFormat);
}

void CalendarView::setTaskModel(TaskModel *model)
{
    m_model = model;
    if (m_model) {
        connect(m_model, &TaskModel::modelReset, this, &CalendarView::refreshTasks);
        connect(m_model, &TaskModel::layoutChanged, this, &CalendarView::refreshTasks);
        connect(m_model, &TaskModel::rowsInserted, this, &CalendarView::refreshTasks);
        connect(m_model, &TaskModel::rowsRemoved, this, &CalendarView::refreshTasks);
        connect(m_model, &TaskModel::dataChanged, this, &CalendarView::refreshTasks);
        refreshTasks();
    }
}

void CalendarView::refreshTasks()
{
    updateTaskCache();
    update(); // 触发重绘
}

void CalendarView::updateTaskCache()
{
    m_taskCounts.clear();
    m_hasUrgent.clear();

    if (!m_model) return;

    QList<QVariantMap> tasks = m_model->getAllTasks(false); // 获取所有未删除任务

    for (const QVariantMap &task : tasks) {
        // 使用截止日期作为标记点
        QDateTime deadline = task["deadline"].toDateTime();
        if (deadline.isValid()) {
            QDate date = deadline.date();
            m_taskCounts[date]++;

            if (task["priority"].toInt() == 0) { // 紧急任务
                m_hasUrgent[date] = true;
            }
        }
    }
}

void CalendarView::paintCell(QPainter *painter, const QRect &rect, QDate date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    if (m_taskCounts.contains(date)) {
        int count = m_taskCounts[date];
        bool isUrgent = m_hasUrgent.value(date, false);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 绘制小圆点
        int dotSize = 6;
        int margin = 4;

        // 位置：右下角
        QPoint center(rect.right() - margin - dotSize/2, rect.bottom() - margin - dotSize/2);

        QColor dotColor = isUrgent ? QColor("#FF4444") : QColor("#657896");
        painter->setBrush(dotColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(center, dotSize/2, dotSize/2);

        if (count > 1) {
            QFont f = painter->font();
            f.setPixelSize(8);
            painter->setFont(f);
            painter->setPen(QColor("#888"));
            painter->drawText(rect.adjusted(4, 0, 0, -2), Qt::AlignLeft | Qt::AlignBottom, QString::number(count));
        }

        painter->restore();
    }
}
