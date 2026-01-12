#include "exporter.h"
#include "models/taskmodel.h"
#include <QFile>
#include <QTextStream>
#include <QPrinter>
#include <QTextDocument>
#include <QDateTime>
#include <QBuffer>
#include <QTextOption>

bool Exporter::exportTasksToCSV(const QString &filePath, TaskModel *model, const StatisticModel::Filter &f)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << QString::fromUtf8("\xEF\xBB\xBF");
    out << "ID,标题,分类,优先级,状态,创建时间,完成时间,截止时间,描述\n";

    QList<QVariantMap> allTasks = model->getAllTasks(false);
    for (const QVariantMap &task : allTasks) {
        QDateTime dl = task["deadline"].toDateTime();
        if (dl < f.start || dl > f.end) continue;
        if (!f.categoryIds.isEmpty() && !f.categoryIds.contains(task["category_id"].toInt())) continue;

        QStringList row;
        row << task["id"].toString();
        row << "\"" + task["title"].toString().replace("\"", "\"\"") + "\"";

        QString catName = task["category_name"].toString();
        row << (catName.isEmpty() ? "未分类" : catName);

        int p = task["priority"].toInt();
        row << (p==0?"紧急": (p==1?"重要": (p==2?"普通":"不急")));

        int s = task["status"].toInt();
        row << (s==0?"待办": (s==1?"进行中": (s==2?"已完成":"已延期")));

        row << task["created_at"].toDateTime().toString("yyyy-MM-dd HH:mm");

        QDateTime compAt = task["completed_at"].toDateTime();
        row << (compAt.isValid() ? compAt.toString("yyyy-MM-dd HH:mm") : "-");

        row << dl.toString("yyyy-MM-dd HH:mm");
        row << "\"" + task["description"].toString().replace("\"", "\"\"").replace("\n", " ") + "\"";

        out << row.join(",") << "\n";
    }
    file.close();
    return true;
}

bool Exporter::exportReportToPDF(const QString &filePath, TaskModel *taskModel, StatisticModel *statModel,
                                 const StatisticModel::Filter &f, const QList<QPixmap> &chartPixmaps,
                                 const QString &aiAnalysisText)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);

    QVariantMap stats = statModel->getOverviewStats(f);
    QTextDocument doc;

    auto pixmapToBase64 = [](const QPixmap &pix) {
        QByteArray ba;
        QBuffer buf(&ba);
        pix.save(&buf, "PNG");
        return QString("data:image/png;base64,") + ba.toBase64();
    };

    const int IMG_FULL_WIDTH = 490;
    const int IMG_HALF_WIDTH = 230;

    QString html = "<html><head><style>"
                   "body { font-family: 'Microsoft YaHei', sans-serif; }"
                   "h1 { color: #2c3e50; text-align: center; margin-bottom: 5px; font-size: 16pt; }"
                   "p.subtitle { text-align: center; color: #7f8c8d; font-size: 9pt; margin-top: 0; margin-bottom: 10px; }"
                   "h2 { color: #34495e; border-left: 5px solid #657896; padding-left: 8px; margin-top: 10px; margin-bottom: 5px; font-size: 12pt; }"
                   "table { width: 100%; border-collapse: collapse; }"
                   "th, td { padding: 3px; }"
                   ".stat-box { font-family: monospace; font-size: 9pt;padding: 6px; border-radius: 4px; }"
                   "</style></head><body>";

    html += "<h1>个人工作统计报表</h1>";
    html += QString("<p class='subtitle'>统计周期: %1 至 %2</p>")
                .arg(f.start.toString("yyyy-MM-dd")).arg(f.end.toString("yyyy-MM-dd"));

    html += "<h2>1. 核心数据概览</h2>";
    html += "<div class='stat-box'>";
    html += "<table width='100%'>";
    html += "<tr>"
            "<td width='33%' style='text-align: left;'>任务总数: <b>" + stats["total"].toString() + "</b></td>"
                                          "<td width='34%' style='text-align: center;'>已完成: <b style='color:#27ae60'>" + stats["completed"].toString() + "</b></td>"
                                              "<td width='33%' style='text-align: right;'>完成率: <b>" + QString::number(stats["rate"].toDouble(), 'f', 1) + "%</b></td>"
                                                                  "</tr>";

    html += "<tr>"
            "<td width='33%' style='text-align: left;'>已逾期: <b style='color:#c0392b'>" + stats["overdue"].toString() + "</b></td>"
                                            "<td width='34%' style='text-align: center;'>平均耗时: <b>" + QString::number(statModel->getAverageCompletionTime(f), 'f', 1) + "h</b></td>"
                                                                                "<td width='33%' style='text-align: right;'>产生灵感: <b>" + QString::number(statModel->getInspirationCount(f)) + "</b></td>"
                                                                   "</tr>";
    html += "</table></div>";

    if (chartPixmaps.size() >= 4) {
        html += "<h2>2. 可视化分析</h2>";
        html += "<table border='0' cellspacing='0' cellpadding='0' style='margin-bottom: 5px;'><tr>";
        html += "<td width='50%' valign='top' align='center'>";
        html += QString("<img src='%1' width='%2'>").arg(pixmapToBase64(chartPixmaps[0])).arg(IMG_HALF_WIDTH);
        html += "</td>";
        html += "<td width='50%' valign='top' style='padding-left: 8px;'>";
        html += "<div style='background-color: #f0f4f8; border: 1px solid #dce4ec; border-radius: 6px; padding: 6px; text-align: left;'>";
        html += "<div style='color: #657896; font-weight: bold; font-size: 9pt; border-bottom: 1px solid #dce4ec; padding-bottom: 2px; margin-bottom: 2px;'>✨ AI 智能分析建议</div>";

        QString processedAiText = aiAnalysisText;
        processedAiText.replace("\n", "<br>");
        if(processedAiText.isEmpty()) processedAiText = "<span style='color:#95a5a6; font-style:italic; font-size: 8pt;'>暂无分析内容。</span>";

        html += "<div style='font-size: 8pt; line-height: 1.2; color: #2c3e50;'>" + processedAiText + "</div>";
        html += "</div>";
        html += "</td>";
        html += "</tr></table>";
        html += "<div align='center' style='margin-bottom: 5px; border: 1px solid #eee; padding: 2px; border-radius: 4px;'>";
        html += QString("<img src='%1' width='%2'>").arg(pixmapToBase64(chartPixmaps[1])).arg(IMG_FULL_WIDTH);
        html += "</div>";

        html += "<table border='0' cellspacing='0' cellpadding='0'><tr>";
        html += "<td width='50%' align='center'>";
        html += QString("<img src='%1' width='%2'>").arg(pixmapToBase64(chartPixmaps[2])).arg(IMG_HALF_WIDTH);
        html += "</td>";
        html += "<td width='50%' align='center'>";
        html += QString("<img src='%1' width='%2'>").arg(pixmapToBase64(chartPixmaps[3])).arg(IMG_HALF_WIDTH);
        html += "</td>";
        html += "</tr></table>";
    }

    html += "<h2 style='page-break-before: always; margin-top: 20px;'>3. 任务清单详情</h2>";

    html += "<table border='1' cellspacing='0' cellpadding='2' "
            "style='border-color: #bdc3c7; font-size: 8pt;'>";

    html += "<tr style='background-color: #657896; color: white;'>"
            "<th width='30%'>标题</th>"
            "<th width='15%' style='white-space: nowrap;'>分类</th>"
            "<th width='10%' style='white-space: nowrap;'>状态</th>"
            "<th width='15%' style='white-space: nowrap;'>创建日期</th>"
            "<th width='15%' style='white-space: nowrap;'>完成日期</th>"
            "<th width='15%' style='white-space: nowrap;'>截止日期</th>"
            "</tr>";

    QList<QVariantMap> allTasks = taskModel->getAllTasks(false);
    int rowCount = 0;

    for (const QVariantMap &task : allTasks) {
        QDateTime dl = task["deadline"].toDateTime();
        if (dl < f.start || dl > f.end) continue;
        if (!f.categoryIds.isEmpty() && !f.categoryIds.contains(task["category_id"].toInt())) continue;

        int s = task["status"].toInt();
        QString sText = (s==0?"待办": (s==1?"进行中": (s==2?"已完成":"已延期")));
        QString sColor = (s==0?"#e67e22": (s==1?"#3498db": (s==2?"#27ae60":"#c0392b")));

        QString catName = task["category_name"].toString();
        if (catName.isEmpty()) catName = "未分类";

        QString createStr = task["created_at"].toDateTime().toString("yyyy-MM-dd");
        QDateTime compAt = task["completed_at"].toDateTime();
        QString compStr = compAt.isValid() ? compAt.toString("yyyy-MM-dd") : "-";

        QString bgColor = (rowCount % 2 == 0) ? "#ffffff" : "#f2f6f8";

        html += QString("<tr style='background-color: %1;'>"
                        "<td style='text-align: left;'>%2</td>"
                        "<td>%3</td>"
                        "<td style='color: %4; font-weight: bold;'>%5</td>"
                        "<td>%6</td>"
                        "<td>%7</td>"
                        "<td>%8</td>"
                        "</tr>")
                    .arg(bgColor)
                    .arg(task["title"].toString())
                    .arg(catName)
                    .arg(sColor)
                    .arg(sText)
                    .arg(createStr)
                    .arg(compStr)
                    .arg(dl.toString("yyyy-MM-dd"));

        rowCount++;
    }

    html += "</table>";

    if (rowCount == 0) {
        html += "<div style='text-align: center; padding: 20px; color: #95a5a6; font-size: 9pt;'>暂无任务记录</div>";
    }

    html += QString("<div style='margin-top: 15px; text-align: right; color: #95a5a6; font-size: 8pt;'>"
                    "生成时间: %1 | 共 %2 条记录"
                    "</div>")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                .arg(rowCount);

    html += "</body></html>";

    doc.setHtml(html);
    QTextOption option;
    option.setWrapMode(QTextOption::WordWrap);
    doc.setDefaultTextOption(option);
    doc.setPageSize(printer.pageRect(QPrinter::Point).size());
    doc.print(&printer);

    return true;
}
