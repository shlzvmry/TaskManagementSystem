#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H

#include <QCalendarWidget>
#include <QMap>
#include <QDate>

class TaskModel;

class CalendarView : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit CalendarView(QWidget *parent = nullptr);
    void setTaskModel(TaskModel *model);
    void refreshTasks();

protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;

private:
    TaskModel *m_model;
    QMap<QDate, int> m_taskCounts; // 日期 -> 任务数量
    QMap<QDate, bool> m_hasUrgent; // 日期 -> 是否有紧急任务

    void updateTaskCache();
};

#endif // CALENDARVIEW_H
