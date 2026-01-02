#include "taskmodel.h"
#include "database/database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMimeData>
#include <QDataStream>

static QSqlDatabase getDbConnection()
{
    QSqlDatabase db = Database::instance().getDatabase();
    if (!db.isOpen()) {
        Database::instance().ensureConnected();
        db = Database::instance().getDatabase();
    }
    return db;
}

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
    , showingDeleted(false)
{
    refresh(false);
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
    return 8;
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
        case 5: return task.deadline.isValid() ? task.deadline.toString("yyyy-MM-dd HH:mm") : "-";
        case 6: return task.remindTime.isValid() ? task.remindTime.toString("yyyy-MM-dd HH:mm") : "-";
        case 7: {
            if (task.status == 2) {
                return task.completedAt.isValid() ? task.completedAt.toString("yyyy-MM-dd HH:mm") : "-";
            }
            return task.createdAt.isValid() ? task.createdAt.toString("yyyy-MM-dd HH:mm") : "-";
        }
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
        case 6: return "提醒时间";
        case 7: return "创建/完成时间";
        default: return QVariant();
        }
    }
    return QVariant();
}

void TaskModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    std::sort(tasks.begin(), tasks.end(), [column, order](const TaskItem &a, const TaskItem &b) {
        bool asc = (order == Qt::AscendingOrder);
        switch (column) {
        case 0: return asc ? a.id < b.id : a.id > b.id;
        case 1: return asc ? a.title < b.title : a.title > b.title;
        case 2: return asc ? a.categoryName < b.categoryName : a.categoryName > b.categoryName;
        case 3: return asc ? a.priority < b.priority : a.priority > b.priority;
        case 4: return asc ? a.status < b.status : a.status > b.status;
        case 5: return asc ? a.deadline < b.deadline : a.deadline > b.deadline;
        case 6: return asc ? a.remindTime < b.remindTime : a.remindTime > b.remindTime;
        case 7: return asc ? a.createdAt < b.createdAt : a.createdAt > b.createdAt;
        default: return asc ? a.id < b.id : a.id > b.id;
        }
    });
    emit layoutChanged();
}

QList<int> TaskModel::resolveTagIds(const QStringList &tagNames, const QStringList &tagColors)
{
    QList<int> tagIds;
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return tagIds;

    for (int i = 0; i < tagNames.size(); ++i) {
        QString name = tagNames[i];
        QString color = (i < tagColors.size()) ? tagColors[i] : "#657896";
        QSqlQuery checkQuery(db);
        checkQuery.prepare("SELECT id FROM task_tags WHERE name = ?");
        checkQuery.addBindValue(name);
        if (checkQuery.exec() && checkQuery.next()) {
            tagIds.append(checkQuery.value(0).toInt());
        } else {
            QSqlQuery insertQuery(db);
            insertQuery.prepare("INSERT INTO task_tags (name, color) VALUES (?, ?)");
            insertQuery.addBindValue(name);
            insertQuery.addBindValue(color);
            if (insertQuery.exec()) {
                tagIds.append(insertQuery.lastInsertId().toInt());
            }
        }
    }
    return tagIds;
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= tasks.size()) return false;
    int taskId = tasks[index.row()].id;
    const TaskItem &currentItem = tasks[index.row()];
    QVariantMap taskData = currentItem.toVariantMap();
    bool changed = false;

    if (role == PriorityRole || (index.column() == 3 && role == Qt::EditRole)) {
        int newPriority = value.toInt();
        if (newPriority != currentItem.priority) {
            taskData["priority"] = newPriority;
            changed = true;
        }
    }
    else if (role == StatusRole || (index.column() == 4 && role == Qt::EditRole)) {
        int newStatus = value.toInt();
        if (newStatus != currentItem.status) {
            taskData["status"] = newStatus;
            if (newStatus == 2) taskData["completed_at"] = QDateTime::currentDateTime();
            else taskData["completed_at"] = QVariant();
            changed = true;
        }
    }
    if (changed) return updateTask(taskId, taskData);
    return false;
}

QMap<int, QVariant> TaskModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> map = QAbstractTableModel::itemData(index);
    if (index.isValid()) {
        map.insert(IdRole, data(index, IdRole));
    }
    return map;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if (index.column() == 3 || index.column() == 4) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

Qt::DropActions TaskModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions TaskModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList TaskModel::mimeTypes() const
{
    return QStringList() << "application/x-task-id";
}

QMimeData *TaskModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    if (!indexes.isEmpty()) {
        const QModelIndex &index = indexes.first();
        if (index.isValid()) {
            int taskId = data(index, IdRole).toInt();
            stream << taskId;
        }
    }

    mimeData->setData("application/x-task-id", encodedData);
    return mimeData;
}

void TaskModel::refresh(bool showDeleted)
{
    showingDeleted = showDeleted;
    loadTasks(showDeleted);
}

void TaskModel::loadTasks(bool includeDeleted)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return;

    beginResetModel();
    tasks.clear();

    QString queryStr = "SELECT t.*, c.name as category_name, c.color as category_color "
                       "FROM tasks t "
                       "LEFT JOIN task_categories c ON t.category_id = c.id ";
    if (!includeDeleted) queryStr += "WHERE t.is_deleted = 0 ";
    queryStr += "ORDER BY t.priority ASC, t.deadline ASC";

    QSqlQuery query(db);
    if (query.exec(queryStr)) {
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
    }
    endResetModel();
}

TaskItem TaskModel::loadTaskFromDb(int taskId) const
{
    TaskItem task;
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return task;

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

bool TaskModel::addTask(const QVariantMap &taskData)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return false;

    TaskItem task = TaskItem::fromVariantMap(taskData);
    task.createdAt = getCurrentTimestamp();
    task.updatedAt = task.createdAt;

    QStringList tagNames = taskData.value("tag_names").toStringList();
    QStringList tagColors = taskData.value("tag_colors").toStringList();
    task.tagIds = resolveTagIds(tagNames, tagColors);

    QSqlQuery query(db);
    QString sql = QString(
        "INSERT INTO tasks (title, description, category_id, priority, "
        "status, start_time, deadline, remind_time, is_reminded, is_deleted, "
        "created_at, updated_at, completed_at) "
        "VALUES (:title, :description, :category_id, :priority, "
        ":status, :start_time, :deadline, :remind_time, :is_reminded, :is_deleted, "
        ":created_at, :updated_at, :completed_at)"
        );

    query.prepare(sql);
    query.bindValue(":title", task.title);
    query.bindValue(":description", task.description);
    query.bindValue(":category_id", task.categoryId);
    query.bindValue(":priority", task.priority);
    query.bindValue(":status", task.status);
    query.bindValue(":start_time", task.startTime);
    query.bindValue(":deadline", task.deadline);
    query.bindValue(":remind_time", task.remindTime);
    query.bindValue(":is_reminded", task.isReminded);
    query.bindValue(":is_deleted", task.isDeleted);
    query.bindValue(":created_at", task.createdAt);
    query.bindValue(":updated_at", task.updatedAt);
    query.bindValue(":completed_at", task.completedAt);

    if (!query.exec()) return false;
    task.id = query.lastInsertId().toInt();

    if (!task.tagIds.isEmpty()) updateTaskTags(task.id, task.tagIds);

    refresh();
    emit taskAdded(task.id);
    return true;
}

bool TaskModel::updateTask(int taskId, const QVariantMap &taskData)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return false;

    TaskItem task = TaskItem::fromVariantMap(taskData);
    task.id = taskId;
    task.updatedAt = getCurrentTimestamp();

    if (task.status == 2 && !task.completedAt.isValid()) {
        task.completedAt = QDateTime::currentDateTime();
    }

    QStringList tagNames = taskData.value("tag_names").toStringList();
    QStringList tagColors = taskData.value("tag_colors").toStringList();
    task.tagIds = resolveTagIds(tagNames, tagColors);

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

    if (!query.exec()) return false;

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM task_tag_relations WHERE task_id = ?");
    deleteQuery.addBindValue(taskId);
    deleteQuery.exec();

    if (!task.tagIds.isEmpty()) updateTaskTags(taskId, task.tagIds);

    refresh();
    emit taskUpdated(taskId);
    return true;
}

bool TaskModel::deleteTask(int taskId, bool softDelete)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    if (softDelete) {
        query.prepare("UPDATE tasks SET is_deleted = 1, updated_at = ? WHERE id = ?");
        query.addBindValue(getCurrentTimestamp());
        query.addBindValue(taskId);
        if (!query.exec()) return false;
        refresh(showingDeleted);
        emit taskDeleted(taskId);
        return true;
    } else {
        return permanentDeleteTask(taskId);
    }
}

QList<QVariantMap> TaskModel::getDeletedTasks() const
{
    QList<QVariantMap> taskList;
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return taskList;

    QSqlQuery query(db);
    query.prepare("SELECT t.*, c.name as category_name, c.color as category_color "
                  "FROM tasks t "
                  "LEFT JOIN task_categories c ON t.category_id = c.id "
                  "WHERE t.is_deleted = 1 "
                  "ORDER BY t.updated_at DESC");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap task;
            QSqlRecord record = query.record();
            for (int i = 0; i < record.count(); ++i) {
                task[record.fieldName(i)] = query.value(i);
            }
            int taskId = task["id"].toInt();
            QList<int> tagIds = loadTaskTags(taskId);
            QVariantList tagNames, tagColors;
            for (int tagId : tagIds) {
                QSqlQuery tagQuery(db);
                tagQuery.prepare("SELECT name, color FROM task_tags WHERE id = ?");
                tagQuery.addBindValue(tagId);
                if (tagQuery.exec() && tagQuery.next()) {
                    tagNames.append(tagQuery.value("name").toString());
                    tagColors.append(tagQuery.value("color").toString());
                }
            }
            task["tag_ids"] = QVariant::fromValue(tagIds);
            task["tag_names"] = tagNames;
            task["tag_colors"] = tagColors;
            taskList.append(task);
        }
    }
    return taskList;
}

int TaskModel::getDeletedTaskCount() const
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return 0;
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM tasks WHERE is_deleted = 1");
    if (query.exec() && query.next()) return query.value(0).toInt();
    return 0;
}

bool TaskModel::restoreTask(int taskId)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return false;
    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET is_deleted = 0, updated_at = ? WHERE id = ?");
    query.addBindValue(getCurrentTimestamp());
    query.addBindValue(taskId);
    if (!query.exec()) return false;
    if (showingDeleted) refresh(true);
    emit taskRestored(taskId);
    return true;
}

bool TaskModel::permanentDeleteTask(int taskId)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return false;
    db.transaction();
    try {
        QSqlQuery deleteTagsQuery(db);
        deleteTagsQuery.prepare("DELETE FROM task_tag_relations WHERE task_id = ?");
        deleteTagsQuery.addBindValue(taskId);
        if (!deleteTagsQuery.exec()) { db.rollback(); return false; }

        QSqlQuery deleteTaskQuery(db);
        deleteTaskQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteTaskQuery.addBindValue(taskId);
        if (!deleteTaskQuery.exec()) { db.rollback(); return false; }

        db.commit();
        refresh(showingDeleted);
        emit taskPermanentlyDeleted(taskId);
        return true;
    } catch (...) {
        db.rollback();
        return false;
    }
}

QVariantMap TaskModel::getTask(int taskId) const
{
    for (const TaskItem &task : tasks) {
        if (task.id == taskId) return task.toVariantMap();
    }
    TaskItem task = loadTaskFromDb(taskId);
    return task.toVariantMap();
}

QList<int> TaskModel::loadTaskTags(int taskId) const
{
    QList<int> tagIds;
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return tagIds;
    QSqlQuery query(db);
    query.prepare("SELECT tag_id FROM task_tag_relations WHERE task_id = ?");
    query.addBindValue(taskId);
    if (query.exec()) {
        while (query.next()) tagIds.append(query.value("tag_id").toInt());
    }
    return tagIds;
}

bool TaskModel::updateTaskTags(int taskId, const QList<int> &tagIds)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return false;
    bool success = true;
    for (int tagId : tagIds) {
        QSqlQuery query(db);
        query.prepare("INSERT OR IGNORE INTO task_tag_relations (task_id, tag_id) VALUES (?, ?)");
        query.addBindValue(taskId);
        query.addBindValue(tagId);
        if (!query.exec()) success = false;
    }
    return success;
}

QList<QVariantMap> TaskModel::getAllTasks(bool includeDeleted) const
{
    QList<QVariantMap> taskList;
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return taskList;
    QString queryStr = "SELECT * FROM tasks ";
    if (!includeDeleted) queryStr += "WHERE is_deleted = 0 ";
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
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return taskList;
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
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return taskList;
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
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return taskList;
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

QMap<int, QString> TaskModel::getPriorityOptions()
{
    return {{0, "紧急"}, {1, "重要"}, {2, "普通"}, {3, "不急"}};
}

QMap<int, QString> TaskModel::getStatusOptions()
{
    return {{0, "待办"}, {1, "进行中"}, {2, "已完成"}, {3, "已延期"}};
}

int TaskModel::getTaskCount(bool includeDeleted) const
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return 0;
    QString queryStr = "SELECT COUNT(*) FROM tasks ";
    if (!includeDeleted) queryStr += "WHERE is_deleted = 0";
    QSqlQuery query(db);
    if (query.exec(queryStr) && query.next()) return query.value(0).toInt();
    return 0;
}

int TaskModel::getCompletedCount() const
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) return 0;
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM tasks WHERE status = 2 AND is_deleted = 0");
    if (query.exec() && query.next()) return query.value(0).toInt();
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
