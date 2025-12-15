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

    // 基本CRUD操作
    bool executeQuery(const QString& query);
    QSqlQuery executeSelect(const QString& query);

    // 带参数的查询
    QSqlQuery prepareQuery(const QString& query);
    bool executePreparedQuery(QSqlQuery &query);

    // 获取分类和标签
    QList<QVariantMap> getAllCategories() const;
    QList<QVariantMap> getAllTags() const;

    //添加分类和标签
    bool addCategory(const QString &name, const QString &color = "#657896");
    bool addTag(const QString &name, const QString &color = "#657896");

    // 事务处理
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

private:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase db;
    QString dbPath;
};

#endif // DATABASE_H
