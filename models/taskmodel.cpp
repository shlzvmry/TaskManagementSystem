#include "taskmodel.h"
#include "database/database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QColor>
#include <QSqlRecord>

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    db = Database::instance().getDatabase();
    refresh();
}

TaskModel::~TaskModel()
{
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return tasks.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 7; // ID, 标题, 分类, 优先级, 状态, 截止时间, 创建时间
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= tasks.size())
        return QVariant();

    const TaskItem &task = tasks.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: return task.id;
        case 1: return task.title;
        case 2: return task.categoryName;
        case 3: return task.priorityText();
        case 4: return task.statusText();
        case 5: return task.deadline.toString("yyyy-MM-dd HH:mm");
        case 6: return task.createdAt.toString("yyyy-MM-dd HH:mm");
        default: return QVariant();
        }

    case Qt::DecorationRole:
        if (index.column() == 3) return task.priorityColor();
        if (index.column() == 4) return task.statusColor();
        break;

    case Qt::ToolTipRole:
        return QString("描述: %1\n标签: %2")
            .arg(task.description)
            .arg(task.tagNames.join(", "));

    case Qt::TextAlignmentRole:
        return int(Qt::AlignCenter);

    case IdRole: return task.id;
    case TitleRole: return task.title;
    case DescriptionRole: return task.description;
    case CategoryIdRole: return task.categoryId;
    case CategoryNameRole: return task.categoryName;
    case CategoryColorRole: return task.categoryColor;
    case PriorityRole: return task.priority;
    case PriorityTextRole: return task.priorityText();
    case PriorityColorRole: return task.priorityColor();
    case StatusRole: return task.status;
    case StatusTextRole: return task.statusText();
    case StatusColorRole: return task.statusColor();
    case StartTimeRole: return task.startTime;
    case DeadlineRole: return task.deadline;
    case RemindTimeRole: return task.remindTime;
    case IsRemindedRole: return task.isReminded;
    case IsDeletedRole: return task.isDeleted;
    case CreatedAtRole: return task.createdAt;
    case UpdatedAtRole: return task.updatedAt;
    case CompletedAtRole: return task.completedAt;
    case TagIdsRole: return QVariant::fromValue(task.tagIds);
    case TagNamesRole: return QVariant::fromValue(task.tagNames);
    case TagColorsRole: return QVariant::fromValue(task.tagColors);
    case IsOverdueRole: return task.isOverdue();
    }

    return QVariant();
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "ID";
        case 1: return "标题";
        case 2: return "分类";
        case 3: return "优先级";
        case 4: return "状态";
        case 5: return "截止时间";
        case 6: return "创建时间";
        default: return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= tasks.size())
        return false;

    TaskItem &task = tasks[index.row()];
    bool success = false;

    switch (role) {
    case StatusRole:
        task.status = value.toInt();
        if (task.status == 2 && !task.completedAt.isValid()) {
            task.completedAt = QDateTime::currentDateTime();
        }
        success = updateTask(task.id, task.toVariantMap());
        break;
    case PriorityRole:
        task.priority = value.toInt();
        success = updateTask(task.id, task.toVariantMap());
        break;
    default:
        return false;
    }

    if (success) {
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

void TaskModel::loadTasks(bool includeDeleted)
{
    beginResetModel();
    tasks.clear();

    QString queryStr = "SELECT t.*, c.name as category_name, c.color as category_color "
                       "FROM tasks t "
                       "LEFT JOIN task_categories c ON t.category_id = c.id ";

    if (!includeDeleted) {
        queryStr += "WHERE t.is_deleted = 0 ";
    }

    queryStr += "ORDER BY t.priority ASC, t.deadline ASC";

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        qDebug() << "加载任务失败:" << query.lastError().text();
        endResetModel();
        return;
    }

    while (query.next()) {
        TaskItem task;
        task.id = query.value("id").toInt();
        task.title = query.value("title").toString();
        task.description = query.value("description").toString();
        task.categoryId = query.value("category_id").toInt();
        task.categoryName = query.value("category_name").toString();
        task.categoryColor = query.value("category_color").toString();
        task.priority = query.value("priority").toInt();
        task.status = query.value("status").toInt();
        task.startTime = query.value("start_time").toDateTime();
        task.deadline = query.value("deadline").toDateTime();
        task.remindTime = query.value("remind_time").toDateTime();
        task.isReminded = query.value("is_reminded").toBool();
        task.isDeleted = query.value("is_deleted").toBool();
        task.createdAt = query.value("created_at").toDateTime();
        task.updatedAt = query.value("updated_at").toDateTime();
        task.completedAt = query.value("completed_at").toDateTime();

        // 加载标签
        task.tagIds = loadTaskTags(task.id);
        for (int tagId : task.tagIds) {
            QSqlQuery tagQuery(db);
            tagQuery.prepare("SELECT name, color FROM task_tags WHERE id = ?");
            tagQuery.addBindValue(tagId);
            if (tagQuery.exec() && tagQuery.next()) {
                task.tagNames.append(tagQuery.value("name").toString());
                task.tagColors.append(tagQuery.value("color").toString());
            }
        }

        tasks.append(task);
    }

    endResetModel();
}

TaskItem TaskModel::loadTaskFromDb(int taskId) const
{
    TaskItem task;

    QSqlQuery query(db);
    query.prepare("SELECT t.*, c.name as category_name, c.color as category_color "
                  "FROM tasks t "
                  "LEFT JOIN task_categories c ON t.category_id = c.id "
                  "WHERE t.id = ?");
    query.addBindValue(taskId);

    if (query.exec() && query.next()) {
        task.id = query.value("id").toInt();
        task.title = query.value("title").toString();
        task.description = query.value("description").toString();
        task.categoryId = query.value("category_id").toInt();
        task.categoryName = query.value("category_name").toString();
        task.categoryColor = query.value("category_color").toString();
        task.priority = query.value("priority").toInt();
        task.status = query.value("status").toInt();
        task.startTime = query.value("start_time").toDateTime();
        task.deadline = query.value("deadline").toDateTime();
        task.remindTime = query.value("remind_time").toDateTime();
        task.isReminded = query.value("is_reminded").toBool();
        task.isDeleted = query.value("is_deleted").toBool();
        task.createdAt = query.value("created_at").toDateTime();
        task.updatedAt = query.value("updated_at").toDateTime();
        task.completedAt = query.value("completed_at").toDateTime();

        // 加载标签
        task.tagIds = loadTaskTags(taskId);
        for (int tagId : task.tagIds) {
            QSqlQuery tagQuery(db);
            tagQuery.prepare("SELECT name, color FROM task_tags WHERE id = ?");
            tagQuery.addBindValue(tagId);
            if (tagQuery.exec() && tagQuery.next()) {
                task.tagNames.append(tagQuery.value("name").toString());
                task.tagColors.append(tagQuery.value("color").toString());
            }
        }
    }

    return task;
}

QList<int> TaskModel::loadTaskTags(int taskId) const
{
    QList<int> tagIds;

    QSqlQuery query(db);
    query.prepare("SELECT tag_id FROM task_tag_relations WHERE task_id = ?");
    query.addBindValue(taskId);

    if (query.exec()) {
        while (query.next()) {
            tagIds.append(query.value("tag_id").toInt());
        }
    }

    return tagIds;
}

bool TaskModel::addTask(const QVariantMap &taskData)
{
    TaskItem task = TaskItem::fromVariantMap(taskData);
    task.createdAt = getCurrentTimestamp();
    task.updatedAt = task.createdAt;

    QSqlQuery query(db);
    query.prepare("INSERT INTO tasks (title, description, category_id, priority, "
                  "status, start_time, deadline, remind_time, is_reminded, is_deleted, "
                  "created_at, updated_at, completed_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(task.title);
    query.addBindValue(task.description);
    query.addBindValue(task.categoryId);
    query.addBindValue(task.priority);
    query.addBindValue(task.status);
    query.addBindValue(task.startTime);
    query.addBindValue(task.deadline);
    query.addBindValue(task.remindTime);
    query.addBindValue(task.isReminded);
    query.addBindValue(task.isDeleted);
    query.addBindValue(task.createdAt);
    query.addBindValue(task.updatedAt);
    query.addBindValue(task.completedAt);

    if (!query.exec()) {
        qDebug() << "添加任务失败:" << query.lastError().text();
        return false;
    }

    task.id = query.lastInsertId().toInt();

    // 保存标签
    if (!task.tagIds.isEmpty()) {
        updateTaskTags(task.id, task.tagIds);
    }

    refresh();  // 刷新整个模型
    emit taskAdded(task.id);

    return true;
}

bool TaskModel::updateTask(int taskId, const QVariantMap &taskData)
{
    TaskItem task = TaskItem::fromVariantMap(taskData);
    task.id = taskId;
    task.updatedAt = getCurrentTimestamp();

    if (task.status == 2 && !task.completedAt.isValid()) {
        task.completedAt = QDateTime::currentDateTime();
    }

    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET title = ?, description = ?, category_id = ?, "
                  "priority = ?, status = ?, start_time = ?, deadline = ?, "
                  "remind_time = ?, is_reminded = ?, is_deleted = ?, "
                  "updated_at = ?, completed_at = ? WHERE id = ?");

    query.addBindValue(task.title);
    query.addBindValue(task.description);
    query.addBindValue(task.categoryId);
    query.addBindValue(task.priority);
    query.addBindValue(task.status);
    query.addBindValue(task.startTime);
    query.addBindValue(task.deadline);
    query.addBindValue(task.remindTime);
    query.addBindValue(task.isReminded);
    query.addBindValue(task.isDeleted);
    query.addBindValue(task.updatedAt);
    query.addBindValue(task.completedAt);
    query.addBindValue(taskId);

    if (!query.exec()) {
        qDebug() << "更新任务失败:" << query.lastError().text();
        return false;
    }

    // 更新标签
    if (!task.tagIds.isEmpty()) {
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM task_tag_relations WHERE task_id = ?");
        deleteQuery.addBindValue(taskId);
        deleteQuery.exec();

        updateTaskTags(taskId, task.tagIds);
    }

    refresh();
    emit taskUpdated(taskId);

    return true;
}

bool TaskModel::updateTaskTags(int taskId, const QList<int> &tagIds)
{
    bool success = true;

    for (int tagId : tagIds) {
        QSqlQuery query(db);
        query.prepare("INSERT OR IGNORE INTO task_tag_relations (task_id, tag_id) VALUES (?, ?)");
        query.addBindValue(taskId);
        query.addBindValue(tagId);

        if (!query.exec()) {
            qDebug() << "更新任务标签失败:" << query.lastError().text();
            success = false;
        }
    }

    return success;
}

bool TaskModel::deleteTask(int taskId, bool softDelete)
{
    QSqlQuery query(db);

    if (softDelete) {
        query.prepare("UPDATE tasks SET is_deleted = 1, updated_at = ? WHERE id = ?");
        query.addBindValue(getCurrentTimestamp());
        query.addBindValue(taskId);
    } else {
        query.prepare("DELETE FROM tasks WHERE id = ?");
        query.addBindValue(taskId);
    }

    if (!query.exec()) {
        qDebug() << "删除任务失败:" << query.lastError().text();
        return false;
    }

    refresh();
    emit taskDeleted(taskId);

    return true;
}

bool TaskModel::restoreTask(int taskId)
{
    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET is_deleted = 0, updated_at = ? WHERE id = ?");
    query.addBindValue(getCurrentTimestamp());
    query.addBindValue(taskId);

    if (!query.exec()) {
        qDebug() << "恢复任务失败:" << query.lastError().text();
        return false;
    }

    refresh();
    emit taskUpdated(taskId);

    return true;
}

QVariantMap TaskModel::getTask(int taskId) const
{
    for (const TaskItem &task : tasks) {
        if (task.id == taskId) {
            return task.toVariantMap();
        }
    }

    // 如果内存中没有，从数据库加载
    TaskItem task = loadTaskFromDb(taskId);
    return task.toVariantMap();
}

QList<QVariantMap> TaskModel::getAllTasks(bool includeDeleted) const
{
    QList<QVariantMap> taskList;

    QString queryStr = "SELECT * FROM tasks ";
    if (!includeDeleted) {
        queryStr += "WHERE is_deleted = 0 ";
    }
    queryStr += "ORDER BY created_at DESC";

    QSqlQuery query(db);
    if (query.exec(queryStr)) {
        while (query.next()) {
            QVariantMap task;
            for (int i = 0; i < query.record().count(); ++i) {
                task[query.record().fieldName(i)] = query.value(i);
            }
            taskList.append(task);
        }
    }

    return taskList;
}

QList<QVariantMap> TaskModel::getTasksByStatus(int status) const
{
    QList<QVariantMap> taskList;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM tasks WHERE status = ? AND is_deleted = 0 ORDER BY deadline ASC");
    query.addBindValue(status);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap task;
            for (int i = 0; i < query.record().count(); ++i) {
                task[query.record().fieldName(i)] = query.value(i);
            }
            taskList.append(task);
        }
    }

    return taskList;
}

QList<QVariantMap> TaskModel::getTasksByCategory(int categoryId) const
{
    QList<QVariantMap> taskList;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM tasks WHERE category_id = ? AND is_deleted = 0 ORDER BY created_at DESC");
    query.addBindValue(categoryId);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap task;
            for (int i = 0; i < query.record().count(); ++i) {
                task[query.record().fieldName(i)] = query.value(i);
            }
            taskList.append(task);
        }
    }

    return taskList;
}

QList<QVariantMap> TaskModel::getTasksByTag(int tagId) const
{
    QList<QVariantMap> taskList;

    QSqlQuery query(db);
    query.prepare("SELECT t.* FROM tasks t "
                  "JOIN task_tag_relations r ON t.id = r.task_id "
                  "WHERE r.tag_id = ? AND t.is_deleted = 0 "
                  "ORDER BY t.created_at DESC");
    query.addBindValue(tagId);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap task;
            for (int i = 0; i < query.record().count(); ++i) {
                task[query.record().fieldName(i)] = query.value(i);
            }
            taskList.append(task);
        }
    }

    return taskList;
}

void TaskModel::refresh()
{
    loadTasks();
}

QMap<int, QString> TaskModel::getPriorityOptions()
{
    return {
        {0, "紧急"},
        {1, "重要"},
        {2, "普通"},
        {3, "不急"}
    };
}

QMap<int, QString> TaskModel::getStatusOptions()
{
    return {
        {0, "待办"},
        {1, "进行中"},
        {2, "已完成"},
        {3, "已延期"}
    };
}

int TaskModel::getTaskCount(bool includeDeleted) const
{
    QString queryStr = "SELECT COUNT(*) FROM tasks ";
    if (!includeDeleted) {
        queryStr += "WHERE is_deleted = 0";
    }

    QSqlQuery query(db);
    if (query.exec(queryStr) && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

int TaskModel::getCompletedCount() const
{
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM tasks WHERE status = 2 AND is_deleted = 0");

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

double TaskModel::getCompletionRate() const
{
    int total = getTaskCount();
    if (total == 0) return 0.0;

    int completed = getCompletedCount();
    return static_cast<double>(completed) / total * 100.0;
}

QDateTime TaskModel::getCurrentTimestamp() const
{
    return QDateTime::currentDateTime();
}
