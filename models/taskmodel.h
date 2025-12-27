#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QList>
#include <QColor>
#include <QSqlRecord>
#include "taskitem.h"

class TaskModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum TaskRole {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        CategoryIdRole,
        CategoryNameRole,
        CategoryColorRole,
        PriorityRole,
        PriorityTextRole,
        PriorityColorRole,
        StatusRole,
        StatusTextRole,
        StatusColorRole,
        StartTimeRole,
        DeadlineRole,
        RemindTimeRole,
        IsRemindedRole,
        IsDeletedRole,
        CreatedAtRole,
        UpdatedAtRole,
        CompletedAtRole,
        TagIdsRole,
        TagNamesRole,
        TagColorsRole,
        IsOverdueRole
    };

    explicit TaskModel(QObject *parent = nullptr);
    ~TaskModel();

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // 自定义操作
    bool addTask(const QVariantMap &taskData);
    bool updateTask(int taskId, const QVariantMap &taskData);
    bool deleteTask(int taskId, bool softDelete = true);
    bool restoreTask(int taskId);
    QVariantMap getTask(int taskId) const;
    bool permanentDeleteTask(int taskId);
    // 排序接口
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // 批量操作
    QList<QVariantMap> getAllTasks(bool includeDeleted = false) const;
    QList<QVariantMap> getTasksByStatus(int status) const;
    QList<QVariantMap> getTasksByCategory(int categoryId) const;
    QList<QVariantMap> getTasksByTag(int tagId) const;
    QList<QVariantMap> getDeletedTasks() const;

    // 刷新数据
    void refresh(bool showDeleted = false);

    // 状态和优先级选项
    static QMap<int, QString> getPriorityOptions();
    static QMap<int, QString> getStatusOptions();

    // 任务统计
    int getTaskCount(bool includeDeleted = false) const;
    int getCompletedCount() const;
    double getCompletionRate() const;

    // 回收站统计
    int getDeletedTaskCount() const;

signals:
    void taskAdded(int taskId);
    void taskUpdated(int taskId);
    void taskDeleted(int taskId);
    void taskRestored(int taskId);
    void taskPermanentlyDeleted(int taskId);
    void refreshRequested(bool showDeleted);

private:
    QList<TaskItem> tasks;
    QSqlDatabase db;
    bool showingDeleted;

    // 新增：标签处理辅助函数
    QList<int> resolveTagIds(const QStringList &tagNames, const QStringList &tagColors);

    void loadTasks(bool includeDeleted = false);
    TaskItem loadTaskFromDb(int taskId) const;
    QList<int> loadTaskTags(int taskId) const;
    bool updateTaskTags(int taskId, const QList<int> &tagIds);
    QDateTime getCurrentTimestamp() const;
};

#endif // TASKMODEL_H
