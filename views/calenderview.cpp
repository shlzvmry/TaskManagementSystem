#include "calenderview.h"
#include <QTableView>
#include <QMouseEvent>
#include <QEvent>
#include "models/taskmodel.h"
#include "models/inspirationmodel.h"
#include <QPainter>
#include <QTextCharFormat>
#include <QDebug>

CalendarView::CalendarView(QWidget *parent)
    : QCalendarWidget(parent)
    , m_model(nullptr)
    , m_inspirationModel(nullptr)
    , m_filterCategoryId(-1)
    , m_filterPriority(-1)
{
    setGridVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(QColor("#99B6E3"));
    setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    setWeekdayTextFormat(Qt::Sunday, weekendFormat);

    if (QTableView *view = getInternalView()) {
        view->viewport()->installEventFilter(this);
        view->setMouseTracking(true);
    }
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

void CalendarView::setInspirationModel(InspirationModel *model)
{
    m_inspirationModel = model;
    if (m_inspirationModel) {
        connect(m_inspirationModel, &InspirationModel::modelReset, this, &CalendarView::refreshTasks);
        connect(m_inspirationModel, &InspirationModel::rowsInserted, this, &CalendarView::refreshTasks);
        connect(m_inspirationModel, &InspirationModel::rowsRemoved, this, &CalendarView::refreshTasks);
        connect(m_inspirationModel, &InspirationModel::dataChanged, this, &CalendarView::refreshTasks);
        refreshTasks();
    }
}

void CalendarView::refreshTasks()
{
    updateTaskCache();
    update();
}

void CalendarView::updateTaskCache()
{
    m_taskStatusColors.clear();
    m_inspirationDates.clear();

    if (m_model && m_filterCategoryId != -2) {
        QList<QVariantMap> tasks = m_model->getAllTasks(false);

        struct DayStats {
            bool hasDelayed = false;
            bool hasInProgress = false;
            bool hasTodo = false;
            int totalCount = 0;
            int completedCount = 0;
        };
        QMap<QDate, DayStats> statsMap;

        for (const QVariantMap &task : tasks) {
            if (m_filterCategoryId != -1) {
                if (task["category_id"].toInt() != m_filterCategoryId) continue;
            }
            if (m_filterPriority != -1) {
                if (task["priority"].toInt() != m_filterPriority) continue;
            }

            QDateTime deadline = task["deadline"].toDateTime();
            if (!deadline.isValid()) continue;

            QDate date = deadline.date();
            DayStats &stats = statsMap[date];
            stats.totalCount++;

            int status = task["status"].toInt();
            if (status == 3) stats.hasDelayed = true;
            else if (status == 1) stats.hasInProgress = true;
            else if (status == 0) stats.hasTodo = true;
            else if (status == 2) stats.completedCount++;
        }

        for (auto it = statsMap.begin(); it != statsMap.end(); ++it) {
            const DayStats &s = it.value();
            QColor color;

            if (s.hasDelayed) color = QColor("#F44336");
            else if (s.hasInProgress) color = QColor("#FF9800");
            else if (s.hasTodo) color = QColor("#2196F3");
            else if (s.totalCount > 0 && s.totalCount == s.completedCount) color = QColor("#4CAF50");

            if (color.isValid()) {
                m_taskStatusColors[it.key()] = color;
            }
        }
    }

    if (m_inspirationModel && (m_filterCategoryId == -1 || m_filterCategoryId == -2)) {
        m_inspirationDates = m_inspirationModel->getDatesWithInspirations(m_inspFilterTags, m_inspFilterMatchAll);
    }
}

void CalendarView::paintCell(QPainter *painter, const QRect &rect, QDate date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    int dotSize = 12;
    int margin = 4;

    m_inspRects.remove(date);
    m_taskRects.remove(date);

    if (m_inspirationDates.contains(date)) {
        QRect inspRect(rect.left() + margin, rect.bottom() - margin - dotSize, dotSize, dotSize);
        m_inspRects[date] = inspRect;

        painter->setBrush(QColor("#9b59b6"));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(inspRect);
    }

    if (m_taskStatusColors.contains(date)) {
        QRect taskRect(rect.right() - margin - dotSize, rect.bottom() - margin - dotSize, dotSize, dotSize);
        m_taskRects[date] = taskRect;

        QColor dotColor = m_taskStatusColors[date];
        painter->setBrush(dotColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(taskRect);
    }

    painter->restore();
}

QTableView* CalendarView::getInternalView() const
{
    return findChild<QTableView*>("qt_calendar_calendarview");
}

bool CalendarView::eventFilter(QObject *watched, QEvent *event)
{
    QTableView *view = getInternalView();
    if (watched == view->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QPoint pos = me->pos();

        int tolerance = 2;

        for (auto it = m_inspRects.begin(); it != m_inspRects.end(); ++it) {
            if (it.value().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos)) {
                emit showInspirations(it.key());
                return true;
            }
        }

        for (auto it = m_taskRects.begin(); it != m_taskRects.end(); ++it) {
            if (it.value().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos)) {
                emit showTasks(it.key());
                return true;
            }
        }
    }
    return QCalendarWidget::eventFilter(watched, event);
}

void CalendarView::setFilter(int categoryId, int priority)
{
    m_filterCategoryId = categoryId;
    m_filterPriority = priority;
    refreshTasks();
}

void CalendarView::setInspirationFilter(const QStringList &tags, bool matchAll)
{
    m_inspFilterTags = tags;
    m_inspFilterMatchAll = matchAll;
    refreshTasks();
}
