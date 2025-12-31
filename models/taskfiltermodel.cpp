#include "taskfiltermodel.h"
#include "models/taskmodel.h"
#include <QDateTime>

TaskFilterModel::TaskFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_filterType(AllTasks)
{
    setDynamicSortFilter(true);
}

void TaskFilterModel::setFilterType(FilterType type)
{
    m_filterType = type;
    invalidateFilter();
}

bool TaskFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    int status = sourceModel()->data(index, TaskModel::StatusRole).toInt();
    bool isDeleted = sourceModel()->data(index, TaskModel::IsDeletedRole).toBool();

    if (isDeleted) return false;

    if (m_filterType == UncompletedTasks) {
        // 显示：待办(0), 进行中(1), 延期(3)
        return status != 2;
    } else if (m_filterType == CompletedTasks) {
        // 显示：已完成(2)
        return status == 2;
    }

    return true;
}

bool TaskFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}
