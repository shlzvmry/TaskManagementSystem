#ifndef TASKFILTERMODEL_H
#define TASKFILTERMODEL_H

#include <QSortFilterProxyModel>
#include <QDate>

class TaskFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum FilterMode {
        FilterAll,
        FilterUncompleted,
        FilterCompleted,
        FilterStatus
    };

    explicit TaskFilterModel(QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void setFilterMode(FilterMode mode);
    void setFilterStatus(int status);
    void setFilterCategory(int categoryId);
    void setFilterPriority(int priority);
    void setFilterText(const QString &text);
    void setFilterDateRange(const QDate &start, const QDate &end);
    void clearDateFilter();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:
    FilterMode m_mode;
    int m_targetStatus;
    int m_categoryId;
    int m_priority;
    QString m_searchText;
    bool m_useDateFilter;
    QDate m_startDate;
    QDate m_endDate;
};

#endif // TASKFILTERMODEL_H
