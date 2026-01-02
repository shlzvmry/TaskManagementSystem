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
{
    setGridVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(QColor("#99B6E3"));
    setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    setWeekdayTextFormat(Qt::Sunday, weekendFormat);

    // 安装事件过滤器到内部的 QTableView 视口，以检测精确点击
    if (QTableView *view = getInternalView()) {
        view->viewport()->installEventFilter(this);
        view->setMouseTracking(true); // 开启鼠标追踪以支持悬停效果
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
    update(); // 触发重绘
}

void CalendarView::updateTaskCache()
{
    m_taskStatusColors.clear();
    m_inspirationDates.clear();

    // 1. 更新任务状态缓存
    if (m_model) {
        QList<QVariantMap> tasks = m_model->getAllTasks(false); // 获取未删除的任务

        // 临时存储每一天的状态统计
        struct DayStats {
            bool hasDelayed = false;
            bool hasInProgress = false;
            bool hasTodo = false;
            int totalCount = 0;
            int completedCount = 0;
        };
        QMap<QDate, DayStats> statsMap;

        for (const QVariantMap &task : tasks) {
            QDateTime deadline = task["deadline"].toDateTime();
            if (!deadline.isValid()) continue;

            QDate date = deadline.date();
            DayStats &stats = statsMap[date];
            stats.totalCount++;

            int status = task["status"].toInt();
            // 0:待办, 1:进行中, 2:已完成, 3:已延期
            if (status == 3) stats.hasDelayed = true;
            else if (status == 1) stats.hasInProgress = true;
            else if (status == 0) stats.hasTodo = true;
            else if (status == 2) stats.completedCount++;
        }

        // 根据规则生成颜色
        for (auto it = statsMap.begin(); it != statsMap.end(); ++it) {
            const DayStats &s = it.value();
            QColor color;

            if (s.hasDelayed) {
                color = QColor("#F44336"); // 红色 (存在已延期)
            } else if (s.hasInProgress) {
                color = QColor("#FF9800"); // 黄色 (存在进行中)
            } else if (s.hasTodo) {
                color = QColor("#2196F3"); // 蓝色 (存在待办)
            } else if (s.totalCount > 0 && s.totalCount == s.completedCount) {
                color = QColor("#4CAF50"); // 绿色 (全部完成)
            }

            if (color.isValid()) {
                m_taskStatusColors[it.key()] = color;
            }
        }
    }

    // 2. 更新灵感缓存
    if (m_inspirationModel) {
        m_inspirationDates = m_inspirationModel->getDatesWithInspirations();
    }
}

void CalendarView::paintCell(QPainter *painter, const QRect &rect, QDate date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    int dotSize = 12;
    int margin = 4;

    // 清理旧数据 (如果是重绘整个控件，可以在 paintEvent 前清理，但这里是逐格绘制)
    // 为了简单，我们直接覆盖
    m_inspRects.remove(date);
    m_taskRects.remove(date);

    // 1. 绘制灵感点 (左下角，紫色)
    if (m_inspirationDates.contains(date)) {
        QRect inspRect(rect.left() + margin, rect.bottom() - margin - dotSize, dotSize, dotSize);
        m_inspRects[date] = inspRect; // 缓存区域

        painter->setBrush(QColor("#9b59b6"));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(inspRect);
    }

    // 2. 绘制任务点 (右下角，根据状态变色)
    if (m_taskStatusColors.contains(date)) {
        QRect taskRect(rect.right() - margin - dotSize, rect.bottom() - margin - dotSize, dotSize, dotSize);
        m_taskRects[date] = taskRect; // 缓存区域

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

        // 扩大点击判定范围 (+2px)
        int tolerance = 2;

        // 检查灵感点点击
        for (auto it = m_inspRects.begin(); it != m_inspRects.end(); ++it) {
            if (it.value().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos)) {
                emit showInspirations(it.key());
                return true; // 拦截
            }
        }

        // 检查任务点点击
        for (auto it = m_taskRects.begin(); it != m_taskRects.end(); ++it) {
            if (it.value().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos)) {
                emit showTasks(it.key());
                return true; // 拦截
            }
        }
    }
    return QCalendarWidget::eventFilter(watched, event);
}
