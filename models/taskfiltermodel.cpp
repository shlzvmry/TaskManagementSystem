#include "taskfiltermodel.h"
#include "models/taskmodel.h"
#include <QDateTime>
#include <QDebug>
#include <QMimeData>

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

    bool isDeleted = sourceModel()->data(index, TaskModel::IsDeletedRole).toBool();
    if (isDeleted) return false;

    int status = sourceModel()->data(index, TaskModel::StatusRole).toInt();

    if (m_mode == FilterUncompleted) {
        if (status == 2) return false;
    } else if (m_mode == FilterCompleted) {
        if (status != 2) return false;
    } else if (m_mode == FilterStatus) {
        if (status != m_targetStatus) return false;
    }

    if (m_categoryId != -1) {
        int catId = sourceModel()->data(index, TaskModel::CategoryIdRole).toInt();
        if (catId != m_categoryId) return false;
    }

    if (m_priority != -1) {
        int pri = sourceModel()->data(index, TaskModel::PriorityRole).toInt();
        if (pri != m_priority) return false;
    }

    if (!m_searchText.isEmpty()) {
        QString title = sourceModel()->data(index, TaskModel::TitleRole).toString().toLower();
        QString desc = sourceModel()->data(index, TaskModel::DescriptionRole).toString().toLower();
        if (!title.contains(m_searchText) && !desc.contains(m_searchText)) {
            return false;
        }
    }
    if (m_useDateFilter) {
        QDateTime deadline = sourceModel()->data(index, TaskModel::DeadlineRole).toDateTime();
        if (!deadline.isValid()) return false;
        QDate date = deadline.date();
        if (date < m_startDate || date > m_endDate) return false;
    }

    return true;
}

bool TaskFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (source_left.column() == 3) {
        int leftPriority = sourceModel()->data(source_left, TaskModel::PriorityRole).toInt();
        int rightPriority = sourceModel()->data(source_right, TaskModel::PriorityRole).toInt();
        return leftPriority < rightPriority;
    }

    if (source_left.column() == 4) {
        int leftStatus = sourceModel()->data(source_left, TaskModel::StatusRole).toInt();
        int rightStatus = sourceModel()->data(source_right, TaskModel::StatusRole).toInt();
        return leftStatus < rightStatus;
    }

    if (source_left.column() == 5 || source_left.column() == 6 || source_left.column() == 7) {
        QString leftStr = sourceModel()->data(source_left, Qt::DisplayRole).toString();
        QString rightStr = sourceModel()->data(source_right, Qt::DisplayRole).toString();
        if (leftStr == "-") return false;
        if (rightStr == "-") return true;
        return leftStr < rightStr;
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

QVariant TaskFilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (m_mode == FilterCompleted && section == 7) {
            return "完成时间";
        }
    }
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

Qt::ItemFlags TaskFilterModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags | Qt::ItemIsDropEnabled;

    return QSortFilterProxyModel::flags(index);
}

Qt::DropActions TaskFilterModel::supportedDropActions() const
{
    if (sourceModel())
        return sourceModel()->supportedDropActions();
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions TaskFilterModel::supportedDragActions() const
{
    if (sourceModel())
        return sourceModel()->supportedDragActions();
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList TaskFilterModel::mimeTypes() const
{
    if (sourceModel())
        return sourceModel()->mimeTypes();
    return QStringList();
}

QMimeData *TaskFilterModel::mimeData(const QModelIndexList &indexes) const
{
    if (sourceModel()) {
        QModelIndexList sourceIndexes;
        for (const QModelIndex &index : indexes) {
            sourceIndexes << mapToSource(index);
        }
        return sourceModel()->mimeData(sourceIndexes);
    }
    return nullptr;
}
