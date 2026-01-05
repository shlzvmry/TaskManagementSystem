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
    // 辅助功能接口
    bool backupDatabase(const QString &destPath);
    bool restoreDatabase(const QString &srcPath);

    // 设置存取
    void setSetting(const QString &key, const QString &value);
    QString getSetting(const QString &key, const QString &defaultValue = "");

    // 批量更新逾期任务
    int updateOverdueTasks();

    // 分类管理扩展
    bool clearCategories(); // 清空所有分类（用于初始化重置）
    bool deleteCategory(int id); // 删除分类

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
