#include "taskmodel.h"
#include "database/database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// 辅助函数：获取数据库连接
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
    , showingDeleted(false)  // 默认不显示已删除任务
{
    qDebug() << "TaskModel构造函数";
    refresh(false);
}

TaskModel::~TaskModel()
{
    // 清理资源
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
        case 5: return task.deadline.isValid() ? task.deadline.toString("yyyy-MM-dd HH:mm") : "未设置";
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
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    // 暂时不实现，后续再完善
    return false;
}

void TaskModel::refresh(bool showDeleted)
{
    showingDeleted = showDeleted;
    loadTasks(showDeleted);
}

void TaskModel::loadTasks(bool includeDeleted)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return;
    }

    beginResetModel();
    tasks.clear();

    qDebug() << "\n=== 开始加载任务 ===";
    qDebug() << "数据库连接状态:" << db.isOpen();
    qDebug() << "数据库连接名:" << db.connectionName();

    QString queryStr = "SELECT t.*, c.name as category_name, c.color as category_color "
                       "FROM tasks t "
                       "LEFT JOIN task_categories c ON t.category_id = c.id ";

    if (!includeDeleted) {
        queryStr += "WHERE t.is_deleted = 0 ";
    }

    queryStr += "ORDER BY t.priority ASC, t.deadline ASC";

    qDebug() << "查询SQL:" << queryStr;

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        qDebug() << "加载任务失败! 错误:" << query.lastError().text();
        qDebug() << "错误类型:" << query.lastError().type();
        qDebug() << "数据库文本:" << query.lastError().databaseText();
        qDebug() << "驱动文本:" << query.lastError().driverText();

        endResetModel();
        return;
    }

    int count = 0;
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

        qDebug() << "加载任务" << count << ":" << task.title << "(ID:" << task.id << ")";
        tasks.append(task);
        count++;
    }

    qDebug() << "共加载" << tasks.size() << "个任务";

    endResetModel();
}

TaskItem TaskModel::loadTaskFromDb(int taskId) const
{
    TaskItem task;

    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return task;
    }

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

bool TaskModel::addTask(const QVariantMap &taskData)
{
    qDebug() << "\n=== 开始添加任务 ===";
    qDebug() << "任务数据:" << taskData;

    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return false;
    }

    TaskItem task = TaskItem::fromVariantMap(taskData);
    task.createdAt = getCurrentTimestamp();
    task.updatedAt = task.createdAt;

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

    if (!query.exec()) {
        qDebug() << "添加任务失败! 错误信息:" << query.lastError().text();
        return false;
    }

    task.id = query.lastInsertId().toInt();
    qDebug() << "任务添加成功! 新任务ID:" << task.id;

    // 保存标签
    if (!task.tagIds.isEmpty()) {
        qDebug() << "需要保存的标签ID:" << task.tagIds;
        updateTaskTags(task.id, task.tagIds);
    }

    // 刷新显示
    refresh();
    qDebug() << "模型已刷新，当前任务数:" << tasks.size();

    emit taskAdded(task.id);
    return true;
}

bool TaskModel::updateTask(int taskId, const QVariantMap &taskData)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return false;
    }

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
        // 先删除旧标签
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM task_tag_relations WHERE task_id = ?");
        deleteQuery.addBindValue(taskId);
        deleteQuery.exec();

        // 添加新标签
        updateTaskTags(taskId, task.tagIds);
    }

    refresh();
    emit taskUpdated(taskId);
    return true;
}

bool TaskModel::deleteTask(int taskId, bool softDelete)
{
    qDebug() << "任务ID:" << taskId << ", 软删除:" << softDelete;
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return false;
    }

    QSqlQuery query(db);

    if (softDelete) {
        query.prepare("UPDATE tasks SET is_deleted = 1, updated_at = ? WHERE id = ?");
        query.addBindValue(getCurrentTimestamp());
        query.addBindValue(taskId);

        if (!query.exec()) {
            qDebug() << "软删除任务失败:" << query.lastError().text();
            return false;
        }

        qDebug() << "任务软删除成功，影响行数:" << query.numRowsAffected();

        refresh(showingDeleted);  // 刷新当前视图
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
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return taskList;
    }

    QSqlQuery query(db);
    query.prepare("SELECT t.*, c.name as category_name, c.color as category_color "
                  "FROM tasks t "
                  "LEFT JOIN task_categories c ON t.category_id = c.id "
                  "WHERE t.is_deleted = 1 "
                  "ORDER BY t.updated_at DESC");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap task;
            // 获取所有字段
            QSqlRecord record = query.record();
            for (int i = 0; i < record.count(); ++i) {
                QString fieldName = record.fieldName(i);
                task[fieldName] = query.value(i);
            }

            // 加载标签
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
        qDebug() << "总共找到" << taskList.size() << "个已删除任务";
    } else {
        qDebug() << "查询已删除任务失败:" << query.lastError().text();
    }

    return taskList;
}

int TaskModel::getDeletedTaskCount() const
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM tasks WHERE is_deleted = 1");

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

bool TaskModel::restoreTask(int taskId)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET is_deleted = 0, updated_at = ? WHERE id = ?");
    query.addBindValue(getCurrentTimestamp());
    query.addBindValue(taskId);

    if (!query.exec()) {
        qDebug() << "恢复任务失败:" << query.lastError().text();
        return false;
    }

    // 如果当前显示已删除任务，刷新回收站视图
    if (showingDeleted) {
        refresh(true);
    }

    emit taskRestored(taskId);
    return true;
}

bool TaskModel::permanentDeleteTask(int taskId)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return false;
    }

    // 开始事务
    db.transaction();

    try {
        // 1. 先删除任务标签关联
        QSqlQuery deleteTagsQuery(db);
        deleteTagsQuery.prepare("DELETE FROM task_tag_relations WHERE task_id = ?");
        deleteTagsQuery.addBindValue(taskId);

        if (!deleteTagsQuery.exec()) {
            qDebug() << "删除任务标签关联失败:" << deleteTagsQuery.lastError().text();
            db.rollback();
            return false;
        }

        // 2. 删除任务本身
        QSqlQuery deleteTaskQuery(db);
        deleteTaskQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteTaskQuery.addBindValue(taskId);

        if (!deleteTaskQuery.exec()) {
            qDebug() << "永久删除任务失败:" << deleteTaskQuery.lastError().text();
            db.rollback();
            return false;
        }

        // 提交事务
        db.commit();

        // 刷新当前视图
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
        if (task.id == taskId) {
            return task.toVariantMap();
        }
    }

    // 如果内存中没有，从数据库加载
    TaskItem task = loadTaskFromDb(taskId);
    return task.toVariantMap();
}

QList<int> TaskModel::loadTaskTags(int taskId) const
{
    QList<int> tagIds;

    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return tagIds;
    }

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

bool TaskModel::updateTaskTags(int taskId, const QList<int> &tagIds)
{
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return false;
    }

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

QList<QVariantMap> TaskModel::getAllTasks(bool includeDeleted) const
{
    QList<QVariantMap> taskList;

    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return taskList;
    }

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

    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return taskList;
    }

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
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return taskList;
    }

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
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return taskList;
    }

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
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return 0;
    }

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
    QSqlDatabase db = getDbConnection();
    if (!db.isOpen()) {
        qDebug() << "TaskModel: 数据库未打开";
        return 0;
    }

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
