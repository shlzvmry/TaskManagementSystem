#include "exporter.h"
#include "models/taskmodel.h"
#include <QFile>
#include <QTextStream>
#include <QPrinter>
#include <QTextDocument>
#include <QDateTime>
#include <QDebug>
#include <QPainter>

bool Exporter::exportTasksToCSV(const QString &filePath, TaskModel *model)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    // 写入BOM，防止Excel打开乱码
    out << QString::fromUtf8("\xEF\xBB\xBF");

    // 表头
    out << "ID,标题,分类,优先级,状态,开始时间,截止时间,创建时间,完成时间,描述\n";

    QList<QVariantMap> tasks = model->getAllTasks(false);
    for (const QVariantMap &task : tasks) {
        QStringList row;
        row << task["id"].toString();
        // 处理包含逗号的字段，用引号包裹
        row << "\"" + task["title"].toString().replace("\"", "\"\"") + "\"";
        row << task["category_name"].toString();

        int p = task["priority"].toInt();
        QString pText = (p==0?"紧急": (p==1?"重要": (p==2?"普通":"不急")));
        row << pText;

        int s = task["status"].toInt();
        QString sText = (s==0?"待办": (s==1?"进行中": (s==2?"已完成":"已延期")));
        row << sText;

        row << task["start_time"].toDateTime().toString("yyyy-MM-dd HH:mm");
        row << task["deadline"].toDateTime().toString("yyyy-MM-dd HH:mm");
        row << task["created_at"].toDateTime().toString("yyyy-MM-dd HH:mm");
        row << task["completed_at"].toDateTime().toString("yyyy-MM-dd HH:mm");
        row << "\"" + task["description"].toString().replace("\"", "\"\"").replace("\n", " ") + "\"";

        out << row.join(",") << "\n";
    }

    file.close();
    return true;
}

bool Exporter::exportReportToPDF(const QString &filePath, TaskModel *taskModel, StatisticModel *statModel, const StatisticModel::Filter &f)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);

    // 获取当前筛选条件下的概览数据
    QVariantMap stats = statModel->getOverviewStats(f);

    QTextDocument doc;
    QString html = "<html><body style='font-family: Arial, sans-serif;'>";
    html += "<h1 style='text-align: center; color: #657896;'>任务管理系统统计报表</h1>";
    html += QString("<p style='text-align: center;'>报表周期: %1 至 %2</p>")
                .arg(f.start.toString("yyyy-MM-dd")).arg(f.end.toString("yyyy-MM-dd"));
    html += QString("<p style='text-align: center;'>导出时间: %1</p>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));

    html += "<h2>1. 数据概览</h2>";
    html += "<ul>";
    html += QString("<li>筛选范围内任务总数: %1</li>").arg(stats["total"].toInt());
    html += QString("<li>已完成数: %1</li>").arg(stats["completed"].toInt());
    html += QString("<li>任务完成率: %1%</li>").arg(QString::number(stats["rate"].toDouble(), 'f', 1));
    html += QString("<li>已逾期任务: %1</li>").arg(stats["overdue"].toInt());
    html += QString("<li>平均完成耗时: %1 小时</li>").arg(QString::number(statModel->getAverageCompletionTime(f), 'f', 1));
    html += "</ul>";

    html += "<h2>2. 重点任务清单</h2>";
    html += "<table border='1' cellspacing='0' cellpadding='5' style='width: 100%; border-collapse: collapse;'>";
    html += "<tr style='background-color: #f2f2f2;'><th>标题</th><th>分类</th><th>状态</th><th>截止时间</th></tr>";

    // 注意：这里的任务列表也应符合筛选条件，可以从 taskModel 获取数据后自行过滤，或展示全部
    QList<QVariantMap> allTasks = taskModel->getAllTasks(false);
    int count = 0;
    for (const QVariantMap &task : allTasks) {
        // 简单的时间范围过滤
        QDateTime dl = task["deadline"].toDateTime();
        if (dl < f.start || dl > f.end) continue;

        if (count++ >= 20) break;
        int s = task["status"].toInt();
        QString sText = (s==0?"待办": (s==1?"进行中": (s==2?"已完成":"已延期")));

        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                    .arg(task["title"].toString())
                    .arg(task["category_name"].toString())
                    .arg(sText)
                    .arg(dl.toString("yyyy-MM-dd"));
    }
    html += "</table>";
    html += "</body></html>";

    doc.setHtml(html);
    doc.print(&printer);
    return true;
}
