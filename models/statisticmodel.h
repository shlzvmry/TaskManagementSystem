#ifndef STATISTICMODEL_H
#define STATISTICMODEL_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include <QList>

class StatisticModel : public QObject
{
    Q_OBJECT

public:
    explicit StatisticModel(QObject *parent = nullptr);

    struct Filter {
        QDateTime start;
        QDateTime end;
        QList<int> categoryIds;
    };

    // 核心统计方法
    QVariantMap getOverviewStats(const Filter &f) const;
    QMap<QString, int> getTasksCountByCategory(const Filter &f) const;
    QMap<QString, int> getTasksCountByPriority(const Filter &f) const;
    QMap<QString, int> getTasksCountByStatus(const Filter &f) const;
    QMap<QDate, int> getDailyCompletionTrend(const Filter &f) const;

    // 效率统计：平均完成耗时（小时）
    double getAverageCompletionTime(const Filter &f) const;
    // 灵感统计：该时间段内的灵感数量
    int getInspirationCount(const Filter &f) const;

private:
    QString buildCategoryInClause(const QList<int> &ids) const;
};

#endif // STATISTICMODEL_H
