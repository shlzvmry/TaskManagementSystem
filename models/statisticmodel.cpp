#include "statisticmodel.h"
#include "database/database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

StatisticModel::StatisticModel(QObject *parent) : QObject(parent) {}

QString StatisticModel::buildCategoryInClause(const QList<int> &ids) const
{
    if (ids.isEmpty()) return "";
    QStringList strIds;
    for (int id : ids) strIds << QString::number(id);
    return QString(" AND category_id IN (%1) ").arg(strIds.join(","));
}

QVariantMap StatisticModel::getOverviewStats(const Filter &f) const
{
    QVariantMap res;
    QString catClause = buildCategoryInClause(f.categoryIds);
    QString timeClause = " AND deadline BETWEEN ? AND ? ";

    auto getCount = [&](const QString &extra) {
        QSqlQuery q;
        q.prepare("SELECT COUNT(*) FROM tasks WHERE is_deleted = 0 " + timeClause + catClause + extra);
        q.addBindValue(f.start);
        q.addBindValue(f.end);
        if (q.exec() && q.next()) return q.value(0).toInt();
        return 0;
    };

    res["total"] = getCount("");
    res["completed"] = getCount(" AND status = 2 ");
    res["overdue"] = getCount(" AND status != 2 AND deadline < datetime('now','localtime') ");

    int total = res["total"].toInt();
    res["rate"] = total > 0 ? (double)res["completed"].toInt() / total * 100.0 : 0.0;

    return res;
}

QMap<QString, int> StatisticModel::getTasksCountByCategory(const Filter &f) const
{
    QMap<QString, int> result;
    QSqlQuery q;
    q.prepare("SELECT c.name, COUNT(t.id) as cnt FROM tasks t "
              "JOIN task_categories c ON t.category_id = c.id "
              "WHERE t.is_deleted = 0 AND t.deadline BETWEEN ? AND ? "
              + buildCategoryInClause(f.categoryIds) + " GROUP BY c.name");
    q.addBindValue(f.start);
    q.addBindValue(f.end);
    if (q.exec()) while (q.next()) result[q.value(0).toString()] = q.value(1).toInt();
    return result;
}

QMap<QString, int> StatisticModel::getTasksCountByPriority(const Filter &f) const
{
    QMap<QString, int> result;
    result["紧急"] = 0; result["重要"] = 0; result["普通"] = 0; result["不急"] = 0;

    QStringList names = {"紧急", "重要", "普通", "不急"};
    QSqlQuery q;
    q.prepare("SELECT priority, COUNT(*) FROM tasks WHERE is_deleted = 0 AND deadline BETWEEN ? AND ? "
              + buildCategoryInClause(f.categoryIds) + " GROUP BY priority");
    q.addBindValue(f.start);
    q.addBindValue(f.end);
    if (q.exec()) {
        while (q.next()) {
            int p = q.value(0).toInt();
            if (p >= 0 && p < names.size()) result[names[p]] = q.value(1).toInt();
        }
    }
    return result;
}

QMap<QString, int> StatisticModel::getTasksCountByStatus(const Filter &f) const
{
    QMap<QString, int> result;
    QStringList names = {"待办", "进行中", "已完成", "已延期"};
    QSqlQuery q;
    q.prepare("SELECT status, COUNT(*) FROM tasks WHERE is_deleted = 0 AND deadline BETWEEN ? AND ? "
              + buildCategoryInClause(f.categoryIds) + " GROUP BY status");
    q.addBindValue(f.start);
    q.addBindValue(f.end);
    if (q.exec()) while (q.next()) result[names.value(q.value(0).toInt(), "未知")] = q.value(1).toInt();
    return result;
}

QVector<int> StatisticModel::getHourlyTrend(const Filter &f) const
{
    QVector<int> data(24, 0);
    int currentHour = QDateTime::currentDateTime().time().hour();
    QSqlQuery q;
    q.prepare("SELECT strftime('%H', completed_at) as hour, COUNT(*) FROM tasks "
              "WHERE is_deleted = 0 AND status = 2 AND date(completed_at) = date(?) "
              + buildCategoryInClause(f.categoryIds) + " GROUP BY hour");
    q.addBindValue(f.start);
    if (q.exec()) {
        while (q.next()) {
            int h = q.value(0).toInt();
            if (h >= 0 && h < 24) data[h] = q.value(1).toInt();
        }
    }
    for(int i = currentHour + 1; i < 24; ++i) data[i] = 0;
    return data;
}

QVector<int> StatisticModel::getDailyTrend(const Filter &f) const
{
    int days = f.start.date().daysTo(f.end.date()) + 1;
    if (days <= 0) return QVector<int>();
    QVector<int> data(days, 0);
    QDate today = QDate::currentDate();
    QSqlQuery q;
    q.prepare("SELECT date(completed_at) as d, COUNT(*) FROM tasks "
              "WHERE is_deleted = 0 AND status = 2 AND completed_at BETWEEN ? AND ? "
              + buildCategoryInClause(f.categoryIds) + " GROUP BY d");
    q.addBindValue(f.start);
    q.addBindValue(f.end);
    if (q.exec()) {
        while (q.next()) {
            QDate d = QDate::fromString(q.value(0).toString(), "yyyy-MM-dd");
            int idx = f.start.date().daysTo(d);
            if (idx >= 0 && idx < days) data[idx] = q.value(1).toInt();
        }
    }
    for(int i = 0; i < days; ++i) {
        if (f.start.date().addDays(i) > today) data[i] = 0;
    }
    return data;
}

QVector<int> StatisticModel::getMonthlyTrend(const Filter &f) const
{
    QVector<int> data(12, 0);
    int year = f.start.date().year();

    QSqlQuery q;
    q.prepare("SELECT strftime('%m', completed_at) as m, COUNT(*) FROM tasks "
              "WHERE is_deleted = 0 AND status = 2 "
              "AND strftime('%Y', completed_at) = ? "
              + buildCategoryInClause(f.categoryIds) + " GROUP BY m");
    q.addBindValue(QString::number(year));

    if (q.exec()) {
        while (q.next()) {
            int m = q.value(0).toInt();
            if (m >= 1 && m <= 12) {
                data[m - 1] = q.value(1).toInt();
            }
        }
    }
    return data;
}

double StatisticModel::getAverageCompletionTime(const Filter &f) const
{
    QSqlQuery q;
    q.prepare("SELECT AVG((julianday(completed_at) - julianday(created_at)) * 24) FROM tasks "
              "WHERE is_deleted = 0 AND status = 2 AND completed_at BETWEEN ? AND ? "
              + buildCategoryInClause(f.categoryIds));
    q.addBindValue(f.start);
    q.addBindValue(f.end);
    if (q.exec() && q.next()) return q.value(0).toDouble();
    return 0.0;
}

int StatisticModel::getInspirationCount(const Filter &f) const
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM inspirations WHERE is_deleted = 0 AND created_at BETWEEN ? AND ?");
    q.addBindValue(f.start);
    q.addBindValue(f.end);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}
