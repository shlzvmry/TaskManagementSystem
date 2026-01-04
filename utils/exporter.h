#ifndef EXPORTER_H
#define EXPORTER_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include "models/statisticmodel.h"

class TaskModel;

class Exporter
{
public:
    // 导出任务列表到CSV
    static bool exportTasksToCSV(const QString &filePath, TaskModel *model);

    // 导出报表到PDF，增加 Filter 参数以匹配最新的统计模型
    static bool exportReportToPDF(const QString &filePath, TaskModel *taskModel, StatisticModel *statModel, const StatisticModel::Filter &f);
};

#endif // EXPORTER_H
