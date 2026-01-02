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
    void setFilter(int categoryId, int priority);
    void setInspirationFilter(const QStringList &tags, bool matchAll);

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

    mutable QMap<QDate, QRect> m_inspRects;
    mutable QMap<QDate, QRect> m_taskRects;

    void updateTaskCache();
    QTableView* getInternalView() const;

    int m_filterCategoryId = -1;
    int m_filterPriority = -1;
    QStringList m_inspFilterTags;
    bool m_inspFilterMatchAll = false;
};

#endif // CALENDARVIEW_H
