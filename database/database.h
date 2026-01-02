#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

class Database : public QObject
{
    Q_OBJECT

public:
    static Database& instance();
    bool initDatabase();
    QSqlDatabase getDatabase();
    void setDatabasePath(const QString &path);
    QString getDatabasePath() const;
    bool executeQuery(const QString& query);
    QSqlQuery executeSelect(const QString& query);
    QSqlQuery prepareQuery(const QString& query);
    bool executePreparedQuery(QSqlQuery &query);

    QList<QVariantMap> getAllCategories() const;
    QList<QVariantMap> getAllTags() const;
    bool addCategory(const QString &name, const QString &color = "#657896");
    bool addTag(const QString &name, const QString &color = "#657896");
    bool deleteTag(int tagId);
    bool removeTaskTagRelation(int taskId, int tagId);
    QList<QVariantMap> getTasksByTagId(int tagId);

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool ensureConnected();

private:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void createTables();
    void initDefaultData();

    QSqlDatabase db;
    QString dbPath;
};

#endif // DATABASE_H
