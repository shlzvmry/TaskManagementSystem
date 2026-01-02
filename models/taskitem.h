#ifndef TASKITEM_H
#define TASKITEM_H

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QList>

struct TaskItem {
    int id;
    QString title;
    QString description;
    int categoryId;
    QString categoryName;
    QString categoryColor;
    int priority;
    int status;
    QDateTime startTime;
    QDateTime deadline;
    QDateTime remindTime;
    bool isReminded;
    bool isDeleted;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime completedAt;
    QList<int> tagIds;
    QList<QString> tagNames;
    QList<QString> tagColors;

    QVariantMap toVariantMap() const;
    static TaskItem fromVariantMap(const QVariantMap &data);

    QString priorityText() const;
    QString statusText() const;
    QColor priorityColor() const;
    QColor statusColor() const;
    bool isOverdue() const;
};

#endif // TASKITEM_H
