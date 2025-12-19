#include "database.h"
#include <QVariantMap>
#include <QVariant>
#include <QSqlRecord>

Database::Database(QObject *parent) : QObject(parent)
{
    // 默认使用项目目录下的数据库
    dbPath = "D:/Qt/TaskManagementSystem/task_management.db";
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
    qDebug() << "尝试打开数据库路径:" << dbPath;

    // 检查文件是否存在
    if (!QFile::exists(dbPath)) {
        qDebug() << "数据库文件不存在:" << dbPath;
        return false;
    }

    // 检查文件权限
    QFileInfo fileInfo(dbPath);
    if (!fileInfo.isReadable()) {
        qDebug() << "数据库文件不可读:" << dbPath;
        return false;
    }

    if (!fileInfo.isWritable()) {
        qDebug() << "数据库文件不可写:" << dbPath;
        return false;
    }

    // 先关闭之前的连接（如果有）
    if (db.isOpen()) {
        QString connectionName = db.connectionName();
        db.close();
        db = QSqlDatabase(); // 重置
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 创建数据库连接 - 使用默认连接名
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Database open error:" << db.lastError().text();
        qDebug() << "数据库驱动错误:" << db.lastError().driverText();
        return false;
    }

    // 启用外键约束
    executeQuery("PRAGMA foreign_keys = ON");

    qDebug() << "Database opened successfully at:" << dbPath;
    qDebug() << "数据库连接名:" << db.connectionName();

    return true;
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
