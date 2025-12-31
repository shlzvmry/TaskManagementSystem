#ifndef TASKFILTERMODEL_H
#define TASKFILTERMODEL_H

#include <QSortFilterProxyModel>

class TaskFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum FilterType {
        AllTasks,
        UncompletedTasks, // 待办 + 进行中 + 延期
        CompletedTasks    // 已完成
    };

    explicit TaskFilterModel(QObject *parent = nullptr);

    void setFilterType(FilterType type);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:
    FilterType m_filterType;
};

#endif // TASKFILTERMODEL_H
