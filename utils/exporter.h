#ifndef EXPORTER_H
#define EXPORTER_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QPixmap>
#include "models/statisticmodel.h"

class TaskModel;

class Exporter
{
public:
    static bool exportTasksToCSV(const QString &filePath, TaskModel *model, const StatisticModel::Filter &f);

    static bool exportReportToPDF(const QString &filePath, TaskModel *taskModel, StatisticModel *statModel,
                                  const StatisticModel::Filter &f, const QList<QPixmap> &chartPixmaps,
                                  const QString &aiAnalysisText);
};

#endif // EXPORTER_H
