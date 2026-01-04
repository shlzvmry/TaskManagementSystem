#ifndef SIMPLECHARTWIDGET_H
#define SIMPLECHARTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QDate>

class SimpleChartWidget : public QWidget
{
    Q_OBJECT

public:
    enum ChartType {
        PieChart,
        BarChart,
        LineChart
    };

    explicit SimpleChartWidget(ChartType type, QString title, QWidget *parent = nullptr);

    void setCategoryData(const QMap<QString, int> &data);
    void setTrendData(const QMap<QDate, int> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ChartType m_type;
    QString m_title;
    QMap<QString, int> m_categoryData;
    QMap<QDate, int> m_trendData;

    void drawPieChart(QPainter &painter, const QRect &rect);
    void drawBarChart(QPainter &painter, const QRect &rect);
    void drawLineChart(QPainter &painter, const QRect &rect);

    QColor getColor(int index) const;
};

#endif // SIMPLECHARTWIDGET_H
