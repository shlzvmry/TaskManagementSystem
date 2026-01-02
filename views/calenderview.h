#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H

#include <QCalendarWidget>
#include <QMap>
#include <QDate>

class TaskModel;
class QTableView;

class CalendarView : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit CalendarView(QWidget *parent = nullptr);
    void setTaskModel(TaskModel *model);
    void setInspirationModel(class InspirationModel *model);
    void refreshTasks();

signals:
    void showInspirations(const QDate &date);
    void showTasks(const QDate &date);

protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    TaskModel *m_model;
    class InspirationModel *m_inspirationModel;

    QMap<QDate, QColor> m_taskStatusColors;
    QList<QDate> m_inspirationDates;

    // 新增：缓存用于点击检测
    mutable QMap<QDate, QRect> m_inspRects;
    mutable QMap<QDate, QRect> m_taskRects;

    void updateTaskCache();
    QTableView* getInternalView() const; // 获取内部表格视图
};

#endif // CALENDARVIEW_H
