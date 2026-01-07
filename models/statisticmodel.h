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

    QVariantMap getOverviewStats(const Filter &f) const;
    QMap<QString, int> getTasksCountByCategory(const Filter &f) const;
    QMap<QString, int> getTasksCountByPriority(const Filter &f) const;
    QMap<QString, int> getTasksCountByStatus(const Filter &f) const;
    QMap<QDate, int> getDailyCompletionTrend(const Filter &f) const;
    QVector<int> getHourlyTrend(const Filter &f) const;
    QVector<int> getDailyTrend(const Filter &f) const;
    QVector<int> getMonthlyTrend(const Filter &f) const;

    double getAverageCompletionTime(const Filter &f) const;
    int getInspirationCount(const Filter &f) const;

private:
    QString buildCategoryInClause(const QList<int> &ids) const;
};

#endif // STATISTICMODEL_H
