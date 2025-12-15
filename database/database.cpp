#include "database.h"
#include <QVariantMap>
#include <QVariant>
#include <QSqlRecord>

Database::Database(QObject *parent) : QObject(parent)
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    dbPath = dataDir + "/task_management.db";
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

bool Database::initDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Database open error:" << db.lastError().text();
        return false;
    }

    qDebug() << "Database opened successfully at:" << dbPath;
    return true;
}

QSqlDatabase Database::getDatabase()
{
    return db;
}

bool Database::executeQuery(const QString& query)
{
    QSqlQuery sqlQuery(db);
    if (!sqlQuery.exec(query)) {
        qDebug() << "Query execution error:" << sqlQuery.lastError().text();
        qDebug() << "Query:" << query;
        return false;
    }
    return true;
}

QSqlQuery Database::executeSelect(const QString& query)
{
    QSqlQuery sqlQuery(db);
    sqlQuery.exec(query);
    return sqlQuery;
}

bool Database::beginTransaction()
{
    return db.transaction();
}

bool Database::commitTransaction()
{
    return db.commit();
}

bool Database::rollbackTransaction()
{
    return db.rollback();
}

QSqlQuery Database::prepareQuery(const QString& query)
{
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
