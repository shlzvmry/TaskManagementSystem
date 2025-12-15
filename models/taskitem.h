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
    int priority;           // 0:紧急, 1:重要, 2:普通, 3:不急
    int status;            // 0:待办, 1:进行中, 2:已完成, 3:已延期
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

    // 转换函数
    QVariantMap toVariantMap() const;
    static TaskItem fromVariantMap(const QVariantMap &data);

    // 工具函数
    QString priorityText() const;
    QString statusText() const;
    QColor priorityColor() const;
    QColor statusColor() const;
    bool isOverdue() const;
};

#endif // TASKITEM_H
