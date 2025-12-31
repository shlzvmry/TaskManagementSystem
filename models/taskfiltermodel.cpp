#include "taskfiltermodel.h"
#include "models/taskmodel.h"
#include <QDateTime>
#include <QDebug>

TaskFilterModel::TaskFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_mode(FilterAll)
    , m_targetStatus(0)
    , m_categoryId(-1)
    , m_priority(-1)
    , m_useDateFilter(false)
{
    setDynamicSortFilter(true);
}

void TaskFilterModel::setFilterMode(FilterMode mode)
{
    m_mode = mode;
    invalidateFilter();
}

void TaskFilterModel::setFilterStatus(int status)
{
    m_targetStatus = status;
    if (m_mode == FilterStatus) {
        invalidateFilter();
    }
}

void TaskFilterModel::setFilterCategory(int categoryId)
{
    m_categoryId = categoryId;
    invalidateFilter();
}

void TaskFilterModel::setFilterPriority(int priority)
{
    m_priority = priority;
    invalidateFilter();
}

void TaskFilterModel::setFilterText(const QString &text)
{
    m_searchText = text.trimmed().toLower();
    invalidateFilter();
}

void TaskFilterModel::setFilterDateRange(const QDate &start, const QDate &end)
{
    m_useDateFilter = true;
    m_startDate = start;
    m_endDate = end;
    invalidateFilter();
}

void TaskFilterModel::clearDateFilter()
{
    m_useDateFilter = false;
    invalidateFilter();
}

bool TaskFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    // 1. 基础状态过滤
    bool isDeleted = sourceModel()->data(index, TaskModel::IsDeletedRole).toBool();
    if (isDeleted) return false; // 过滤掉已删除的任务

    int status = sourceModel()->data(index, TaskModel::StatusRole).toInt();

    if (m_mode == FilterUncompleted) {
        if (status == 2) return false; // 过滤掉已完成
    } else if (m_mode == FilterCompleted) {
        if (status != 2) return false; // 只保留已完成
    } else if (m_mode == FilterStatus) {
        if (status != m_targetStatus) return false; // 只保留特定状态
    }

    // 2. 分类过滤
    if (m_categoryId != -1) {
        int catId = sourceModel()->data(index, TaskModel::CategoryIdRole).toInt();
        if (catId != m_categoryId) return false;
    }

    // 3. 优先级过滤
    if (m_priority != -1) {
        int pri = sourceModel()->data(index, TaskModel::PriorityRole).toInt();
        if (pri != m_priority) return false;
    }

    // 4. 文本搜索 (标题或描述)
    if (!m_searchText.isEmpty()) {
        QString title = sourceModel()->data(index, TaskModel::TitleRole).toString().toLower();
        QString desc = sourceModel()->data(index, TaskModel::DescriptionRole).toString().toLower();
        if (!title.contains(m_searchText) && !desc.contains(m_searchText)) {
            return false;
        }
    }

    // 5. 日期过滤 (基于截止日期)
    if (m_useDateFilter) {
        QDateTime deadline = sourceModel()->data(index, TaskModel::DeadlineRole).toDateTime();
        if (!deadline.isValid()) return false; // 无截止日期的任务在日期筛选模式下通常不显示

        QDate date = deadline.date();
        if (date < m_startDate || date > m_endDate) return false;
    }

    return true;
}

bool TaskFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    // 优先级列 (Column 3)
    if (source_left.column() == 3) {
        int leftPriority = sourceModel()->data(source_left, TaskModel::PriorityRole).toInt();
        int rightPriority = sourceModel()->data(source_right, TaskModel::PriorityRole).toInt();
        return leftPriority < rightPriority;
    }

    // 状态列 (Column 4)
    if (source_left.column() == 4) {
        int leftStatus = sourceModel()->data(source_left, TaskModel::StatusRole).toInt();
        int rightStatus = sourceModel()->data(source_right, TaskModel::StatusRole).toInt();
        return leftStatus < rightStatus;
    }

    // 时间列
    if (source_left.column() == 5 || source_left.column() == 6 || source_left.column() == 7) {
        QString leftStr = sourceModel()->data(source_left, Qt::DisplayRole).toString();
        QString rightStr = sourceModel()->data(source_right, Qt::DisplayRole).toString();
        if (leftStr == "-") return false;
        if (rightStr == "-") return true;
        return leftStr < rightStr;
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}
