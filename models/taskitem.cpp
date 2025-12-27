#include "taskitem.h"
#include <QColor>

QVariantMap TaskItem::toVariantMap() const {
    QVariantMap map;
    map["id"] = id;
    map["title"] = title;
    map["description"] = description;
    map["category_id"] = categoryId;
    map["category_name"] = categoryName;
    map["category_color"] = categoryColor;
    map["priority"] = priority;
    map["status"] = status;
    map["start_time"] = startTime;
    map["deadline"] = deadline;
    map["remind_time"] = remindTime;
    map["is_reminded"] = isReminded;
    map["is_deleted"] = isDeleted;
    map["created_at"] = createdAt;
    map["updated_at"] = updatedAt;
    map["completed_at"] = completedAt;

    // 标签处理
    QVariantList tagIdList, tagNameList, tagColorList;
    for (int tagId : tagIds) tagIdList.append(tagId);
    for (const QString &tagName : tagNames) tagNameList.append(tagName);
    for (const QString &tagColor : tagColors) tagColorList.append(tagColor);

    map["tag_ids"] = tagIdList;
    map["tag_names"] = tagNameList;
    map["tag_colors"] = tagColorList;

    return map;
}

TaskItem TaskItem::fromVariantMap(const QVariantMap &data) {
    TaskItem item;
    item.id = data["id"].toInt();
    item.title = data["title"].toString();
    item.description = data["description"].toString();
    item.categoryId = data["category_id"].toInt();
    item.categoryName = data["category_name"].toString();
    item.categoryColor = data["category_color"].toString();
    item.priority = data["priority"].toInt();
    item.status = data["status"].toInt();
    item.startTime = data["start_time"].toDateTime();
    item.deadline = data["deadline"].toDateTime();
    item.remindTime = data["remind_time"].toDateTime();
    item.isReminded = data["is_reminded"].toBool();
    item.isDeleted = data["is_deleted"].toBool();
    item.createdAt = data["created_at"].toDateTime();
    item.updatedAt = data["updated_at"].toDateTime();
    item.completedAt = data["completed_at"].toDateTime();

    // 标签处理
    QVariantList tagIds = data["tag_ids"].toList();
    QVariantList tagNames = data["tag_names"].toList();
    QVariantList tagColors = data["tag_colors"].toList();

    for (const QVariant &tagId : tagIds) item.tagIds.append(tagId.toInt());
    for (const QVariant &tagName : tagNames) item.tagNames.append(tagName.toString());
    for (const QVariant &tagColor : tagColors) item.tagColors.append(tagColor.toString());

    return item;
}

QString TaskItem::priorityText() const {
    switch (priority) {
    case 0: return "紧急";
    case 1: return "重要";
    case 2: return "普通";
    case 3: return "不急";
    default: return "未知";
    }
}

QString TaskItem::statusText() const {
    switch (status) {
    case 0: return "待办";
    case 1: return "进行中";
    case 2: return "已完成";
    case 3: return "已延期";
    default: return "未知";
    }
}

QColor TaskItem::priorityColor() const {
    switch (priority) {
    case 0: return QColor("#C96A6A");
    case 1: return QColor("#D69E68");
    case 2: return QColor("#7FA882");
    case 3: return QColor("#8C949E");
    default: return QColor("#7696B3");
    }
}

QColor TaskItem::statusColor() const {
    switch (status) {
    case 0: return QColor("#7696B3");
    case 1: return QColor("#D69E68");
    case 2: return QColor("#7FA882");
    case 3: return QColor("#C96A6A");
    default: return QColor("#8C949E");
    }
}

bool TaskItem::isOverdue() const {
    if (status == 2) return false; // 已完成的任务不算逾期
    if (!deadline.isValid()) return false;
    return deadline < QDateTime::currentDateTime();
}
