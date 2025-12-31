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

    // 设置自定义数据库路径
    void setDatabasePath(const QString &path);
    QString getDatabasePath() const;

    // 基本CRUD操作
    bool executeQuery(const QString& query);
    QSqlQuery executeSelect(const QString& query);

    // 带参数的查询
    QSqlQuery prepareQuery(const QString& query);
    bool executePreparedQuery(QSqlQuery &query);

    // 获取分类和标签
    QList<QVariantMap> getAllCategories() const;
    QList<QVariantMap> getAllTags() const;

    // 添加分类和标签
    bool addCategory(const QString &name, const QString &color = "#657896");
    bool addTag(const QString &name, const QString &color = "#657896");

    // 标签管理
    bool deleteTag(int tagId);
    bool removeTaskTagRelation(int taskId, int tagId);
    QList<QVariantMap> getTasksByTagId(int tagId);

    // 事务处理
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // 确保数据库连接可用
    bool ensureConnected();

private:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // 初始化表结构
    void createTables();
    // 初始化默认数据
    void initDefaultData();

    QSqlDatabase db;
    QString dbPath;
};

#endif // DATABASE_H
