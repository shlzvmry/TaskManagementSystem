#include "database.h"
#include <QVariantMap>
#include <QVariant>
#include <QSqlRecord>
#include <QCoreApplication>

Database::Database(QObject *parent) : QObject(parent)
{
    dbPath = QCoreApplication::applicationDirPath() + "/task_management.db";
}

Database::~Database()
{
    if (db.isOpen()) {
        db.close();
    }
}

Database& Database::instance()
{
    static Database instance;
    return instance;
}

void Database::setDatabasePath(const QString &path)
{
    dbPath = path;
}

QString Database::getDatabasePath() const
{
    return dbPath;
}

bool Database::initDatabase()
{
    qDebug() << "数据库路径:" << dbPath;

    bool needCreate = !QFile::exists(dbPath);
    if (needCreate) {
        qDebug() << "数据库文件不存在，将自动创建";
    }

    if (db.isOpen()) {
        QString connectionName = db.connectionName();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Database open error:" << db.lastError().text();
        return false;
    }

    executeQuery("PRAGMA foreign_keys = ON");

    if (needCreate) {
        createTables();
        initDefaultData();
    }

    qDebug() << "Database opened successfully";
    return true;
}

void Database::createTables()
{
    executeQuery(
        "CREATE TABLE IF NOT EXISTS task_categories ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE, "
        "color TEXT NOT NULL DEFAULT '#657896', "
        "is_custom INTEGER DEFAULT 0, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
        );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS task_tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE, "
        "color TEXT NOT NULL DEFAULT '#657896', "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
        );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "title TEXT NOT NULL, "
        "description TEXT, "
        "category_id INTEGER, "
        "priority INTEGER DEFAULT 2, "
        "status INTEGER DEFAULT 0, "
        "start_time DATETIME, "
        "deadline DATETIME, "
        "remind_time DATETIME, "
        "is_reminded INTEGER DEFAULT 0, "
        "is_deleted INTEGER DEFAULT 0, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "completed_at DATETIME, "
        "FOREIGN KEY (category_id) REFERENCES task_categories (id))"
        );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS task_tag_relations ("
        "task_id INTEGER NOT NULL, "
        "tag_id INTEGER NOT NULL, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "PRIMARY KEY (task_id, tag_id), "
        "FOREIGN KEY (task_id) REFERENCES tasks (id) ON DELETE CASCADE, "
        "FOREIGN KEY (tag_id) REFERENCES task_tags (id) ON DELETE CASCADE)"
        );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS inspirations ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "content TEXT NOT NULL, "
        "tags TEXT, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "is_deleted INTEGER DEFAULT 0)"
        );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS user_settings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "key TEXT NOT NULL UNIQUE, "
        "value TEXT, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
        );
}

void Database::initDefaultData()
{
    if (getSetting("first_run").isEmpty()) {
        setSetting("first_run", "true");
    }
    if (getSetting("theme_color").isEmpty()) {
        setSetting("theme_color", "#657896");
    }
    if (getSetting("bg_mode").isEmpty()) {
        setSetting("bg_mode", "dark");
    }
    if (getSetting("default_view").isEmpty()) {
        setSetting("default_view", "0");
    }
    if (getSetting("sound_enabled").isEmpty()) {
        setSetting("sound_enabled", "true");
    }
    if (getSetting("popup_enabled").isEmpty()) {
        setSetting("popup_enabled", "true");
    }
    if (getSetting("auto_purge_bin").isEmpty()) {
        setSetting("auto_purge_bin", "false");
    }
    if (getSetting("calendar_start_day").isEmpty()) {
        setSetting("calendar_start_day", "1");
    }
    if (getSetting("default_remind_minutes").isEmpty()) {
        setSetting("default_remind_minutes", "60");
    }
}

QSqlDatabase Database::getDatabase()
{
    return db;
}

bool Database::executeQuery(const QString& query)
{
    if (!db.isOpen()) {
        qDebug() << "数据库未打开，无法执行查询";
        return false;
    }

    QSqlQuery sqlQuery(db);
    if (!sqlQuery.exec(query)) {
        qDebug() << "Query execution error:" << sqlQuery.lastError().text();
        qDebug() << "Query:" << query;
        qDebug() << "Database error:" << sqlQuery.lastError().databaseText();
        return false;
    }
    return true;
}

QSqlQuery Database::executeSelect(const QString& query)
{
    if (!db.isOpen()) {
        qDebug() << "数据库未打开，无法执行查询";
        return QSqlQuery();
    }

    QSqlQuery sqlQuery(db);
    if (!sqlQuery.exec(query)) {
        qDebug() << "Select execution error:" << sqlQuery.lastError().text();
        qDebug() << "Query:" << query;
    }
    return sqlQuery;
}

QSqlQuery Database::prepareQuery(const QString& query)
{
    if (!db.isOpen()) {
        qDebug() << "数据库未打开，无法准备查询";
        return QSqlQuery();
    }

    QSqlQuery sqlQuery(db);
    sqlQuery.prepare(query);
    return sqlQuery;
}

bool Database::executePreparedQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        qDebug() << "预编译查询执行错误:" << query.lastError().text();
        qDebug() << "SQL:" << query.lastQuery();
        return false;
    }
    return true;
}

QList<QVariantMap> Database::getAllCategories() const
{
    QList<QVariantMap> categories;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM task_categories ORDER BY name");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap category;
            for (int i = 0; i < query.record().count(); ++i) {
                category[query.record().fieldName(i)] = query.value(i);
            }
            categories.append(category);
        }
    } else {
        qDebug() << "获取分类失败:" << query.lastError().text();
    }

    return categories;
}

QList<QVariantMap> Database::getAllTags() const
{
    QList<QVariantMap> tags;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM task_tags ORDER BY name");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap tag;
            for (int i = 0; i < query.record().count(); ++i) {
                tag[query.record().fieldName(i)] = query.value(i);
            }
            tags.append(tag);
        }
    } else {
        qDebug() << "获取标签失败:" << query.lastError().text();
    }

    return tags;
}

bool Database::addCategory(const QString &name, const QString &color)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO task_categories (name, color) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(color);

    return executePreparedQuery(query);
}

bool Database::addTag(const QString &name, const QString &color)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO task_tags (name, color) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(color);

    return executePreparedQuery(query);
}

bool Database::deleteTag(int tagId)
{
    if (!db.isOpen()) return false;

    db.transaction();

    QSqlQuery deleteRelationQuery(db);
    deleteRelationQuery.prepare("DELETE FROM task_tag_relations WHERE tag_id = ?");
    deleteRelationQuery.addBindValue(tagId);

    if (!deleteRelationQuery.exec()) {
        qDebug() << "删除标签关联失败:" << deleteRelationQuery.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery deleteTagQuery(db);
    deleteTagQuery.prepare("DELETE FROM task_tags WHERE id = ?");
    deleteTagQuery.addBindValue(tagId);

    if (!deleteTagQuery.exec()) {
        qDebug() << "删除标签失败:" << deleteTagQuery.lastError().text();
        db.rollback();
        return false;
    }

    return db.commit();
}

bool Database::removeTaskTagRelation(int taskId, int tagId)
{
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM task_tag_relations WHERE task_id = ? AND tag_id = ?");
    query.addBindValue(taskId);
    query.addBindValue(tagId);

    if (!query.exec()) {
        qDebug() << "解除标签关联失败:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<QVariantMap> Database::getTasksByTagId(int tagId)
{
    QList<QVariantMap> tasks;
    if (!db.isOpen()) return tasks;

    QSqlQuery query(db);
    query.prepare("SELECT t.id, t.title, c.name as category_name "
                  "FROM tasks t "
                  "JOIN task_tag_relations r ON t.id = r.task_id "
                  "LEFT JOIN task_categories c ON t.category_id = c.id "
                  "WHERE r.tag_id = ? AND t.is_deleted = 0");
    query.addBindValue(tagId);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap task;
            task["id"] = query.value("id");
            task["title"] = query.value("title");
            task["category_name"] = query.value("category_name");
            tasks.append(task);
        }
    }
    return tasks;
}

bool Database::beginTransaction()
{
    if (!db.isOpen()) {
        qDebug() << "数据库未打开，无法开始事务";
        return false;
    }
    return db.transaction();
}

bool Database::commitTransaction()
{
    if (!db.isOpen()) {
        qDebug() << "数据库未打开，无法提交事务";
        return false;
    }
    return db.commit();
}

bool Database::rollbackTransaction()
{
    if (!db.isOpen()) {
        qDebug() << "数据库未打开，无法回滚事务";
        return false;
    }
    return db.rollback();
}

bool Database::ensureConnected()
{
    if (!db.isOpen()) {
        qDebug() << "数据库连接已断开，尝试重新连接";
        return initDatabase();
    }
    return true;
}

bool Database::backupDatabase(const QString &destPath)
{
    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }
    QSqlQuery query(db);
    if (query.exec(QString("VACUUM INTO '%1'").arg(destPath))) {
        return true;
    }

    return QFile::copy(dbPath, destPath);
}

bool Database::restoreDatabase(const QString &srcPath)
{
    if (db.isOpen()) db.close();

    if (QFile::exists(dbPath)) {
        QFile::remove(dbPath);
    }

    if (QFile::copy(srcPath, dbPath)) {
        return initDatabase();
    }
    return false;
}

void Database::setSetting(const QString &key, const QString &value)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO user_settings (key, value, updated_at) VALUES (?, ?, CURRENT_TIMESTAMP)");
    query.addBindValue(key);
    query.addBindValue(value);
    query.exec();
}

QString Database::getSetting(const QString &key, const QString &defaultValue)
{
    QSqlQuery query(db);
    query.prepare("SELECT value FROM user_settings WHERE key = ?");
    query.addBindValue(key);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return defaultValue;
}

int Database::updateOverdueTasks()
{
    if (!db.isOpen()) return 0;
    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET status = 3, updated_at = CURRENT_TIMESTAMP "
                  "WHERE status != 2 AND status != 3 AND deadline < CURRENT_TIMESTAMP AND is_deleted = 0");
    if (query.exec()) {
        return query.numRowsAffected();
    }
    return 0;
}

bool Database::clearCategories()
{
    QSqlQuery query(db);
    return query.exec("DELETE FROM task_categories");
}

bool Database::deleteCategory(int id)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM task_categories WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}
