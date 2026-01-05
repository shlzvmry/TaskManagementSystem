#ifndef SIMPLECHARTWIDGET_H
#define SIMPLECHARTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QDate>

class SimpleChartWidget : public QWidget
{
    Q_OBJECT

public:
    enum ChartType { PieChart, BarChart, LineChart };
    explicit SimpleChartWidget(ChartType type, QString title, QWidget *parent = nullptr);

    void setCategoryData(const QMap<QString, int> &data);
    void setTrendData(const QVector<int> &data, const QStringList &labels, const QStringList &tooltips = QStringList());
    void setSubTitle(const QString &subTitle);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    ChartType m_type;
    QString m_title;
    QString m_subTitle;
    QMap<QString, int> m_categoryData;
    QVector<int> m_trendValues;
    QStringList m_trendLabels;
    QStringList m_tooltips;

    // 用于鼠标悬停交互
    QVector<QPointF> m_currentPoints;
    QPoint m_mousePos;
    bool m_isHovering;

    void drawPieChart(QPainter &painter, const QRect &rect);
    void drawBarChart(QPainter &painter, const QRect &rect);
    void drawLineChart(QPainter &painter, const QRect &rect);

    QColor getColor(int index) const;
};

#endif // SIMPLECHARTWIDGET_H
