#include "simplechartwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QDebug>

SimpleChartWidget::SimpleChartWidget(ChartType type, QString title, QWidget *parent)
    : QWidget(parent), m_type(type), m_title(title)
{
    setMinimumSize(300, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SimpleChartWidget::setCategoryData(const QMap<QString, int> &data)
{
    m_categoryData = data;
    update();
}

void SimpleChartWidget::setTrendData(const QMap<QDate, int> &data)
{
    m_trendData = data;
    update();
}

void SimpleChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 背景绘制：使用圆角矩形，去除黑色长条干扰
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#2d2d2d"));
    painter.drawRoundedRect(rect(), 10, 10);

    // 2. 边框
    painter.setPen(QPen(QColor("#3d3d3d"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);

    // 3. 标题
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(11);
    painter.setFont(font);
    QRect titleRect(0, 10, width(), 30);
    painter.drawText(titleRect, Qt::AlignCenter, m_title);

    // 4. 绘图区域：增加底部边距（从 -20 改为 -45），为标签留出更多空间
    QRect chartRect = rect().adjusted(30, 50, -30, -45);

    if (m_type == PieChart) {
        drawPieChart(painter, chartRect);
    } else if (m_type == BarChart) {
        drawBarChart(painter, chartRect);
    } else if (m_type == LineChart) {
        drawLineChart(painter, chartRect);
    }
}

void SimpleChartWidget::drawPieChart(QPainter &painter, const QRect &rect)
{
    if (m_categoryData.isEmpty()) return;

    int total = 0;
    for (int val : m_categoryData) total += val;
    if (total == 0) return;

    int size = qMin(rect.width(), rect.height());
    // 留出右侧图例空间
    int pieSize = size * 0.8;
    QRect pieRect(rect.left(), rect.top() + (rect.height() - pieSize)/2, pieSize, pieSize);

    int startAngle = 0;
    int colorIndex = 0;

    // 绘制饼图
    for (auto it = m_categoryData.begin(); it != m_categoryData.end(); ++it) {
        if (it.value() == 0) continue;

        int spanAngle = (it.value() * 360 * 16) / total;
        painter.setBrush(getColor(colorIndex));
        painter.setPen(Qt::NoPen);
        painter.drawPie(pieRect, startAngle, spanAngle);

        startAngle += spanAngle;
        colorIndex++;
    }

    // 绘制图例
    int legendX = pieRect.right() + 20;
    int legendY = rect.top() + 20;
    colorIndex = 0;
    QFont legendFont = painter.font();
    legendFont.setPointSize(9);
    legendFont.setBold(false);
    painter.setFont(legendFont);

    for (auto it = m_categoryData.begin(); it != m_categoryData.end(); ++it) {
        if (it.value() == 0) continue;

        painter.setBrush(getColor(colorIndex));
        painter.drawRect(legendX, legendY, 12, 12);

        painter.setPen(QColor("#cccccc"));
        QString text = QString("%1 (%2)").arg(it.key()).arg(it.value());
        painter.drawText(legendX + 20, legendY + 11, text);

        legendY += 25;
        colorIndex++;
    }
}

void SimpleChartWidget::drawBarChart(QPainter &painter, const QRect &rect)
{
    if (m_categoryData.isEmpty()) return;

    int maxVal = 0;
    for (int val : m_categoryData) if (val > maxVal) maxVal = val;
    if (maxVal == 0) maxVal = 1;

    int count = m_categoryData.size();
    int barWidth = (rect.width() / count) * 0.6;
    int spacing = (rect.width() / count) * 0.4;

    int x = rect.left() + spacing / 2;
    int colorIndex = 0;

    QFont labelFont = painter.font();
    labelFont.setPointSize(9);
    labelFont.setBold(false);
    painter.setFont(labelFont);

    for (auto it = m_categoryData.begin(); it != m_categoryData.end(); ++it) {
        int barHeight = (int)((double)it.value() / maxVal * (rect.height() - 20));

        QRect barRect(x, rect.bottom() - barHeight, barWidth, barHeight);

        painter.setBrush(getColor(colorIndex));
        painter.setPen(Qt::NoPen);
        painter.drawRect(barRect);

        // 数值：
        painter.setPen(Qt::white);
        painter.setBackgroundMode(Qt::TransparentMode); // 确保文字背景透明
        painter.drawText(barRect.adjusted(0, -20, 0, 0), Qt::AlignCenter | Qt::AlignBottom, QString::number(it.value()));

        // 标签：下移位置
        painter.setPen(QColor("#cccccc"));
        QRect labelRect(x - 5, rect.bottom() + 10, barWidth + 10, 20);
        painter.drawText(labelRect, Qt::AlignCenter, it.key());

        x += barWidth + spacing;
        colorIndex++;
    }
}

void SimpleChartWidget::drawLineChart(QPainter &painter, const QRect &rect)
{
    if (m_trendData.isEmpty()) return;

    int maxVal = 0;
    for (int val : m_trendData) if (val > maxVal) maxVal = val;
    // Y轴留一点余量
    maxVal = maxVal < 5 ? 5 : maxVal + 1;

    // 绘制坐标轴
    painter.setPen(QColor("#555555"));
    painter.drawLine(rect.bottomLeft(), rect.bottomRight()); // X轴
    painter.drawLine(rect.bottomLeft(), rect.topLeft());     // Y轴

    int count = m_trendData.size();
    if (count < 2) return;

    double stepX = (double)rect.width() / (count - 1);

    QVector<QPointF> points;
    int i = 0;
    for (auto it = m_trendData.begin(); it != m_trendData.end(); ++it) {
        double x = rect.left() + i * stepX;
        double y = rect.bottom() - ((double)it.value() / maxVal * rect.height());
        points.append(QPointF(x, y));

        // 绘制日期标签
        painter.setPen(QColor("#888888"));
        QFont f = painter.font();
        f.setPointSize(8);
        f.setBold(false);
        painter.setFont(f);
        painter.drawText(x - 20, rect.bottom() + 5, 40, 20, Qt::AlignCenter, it.key().toString("MM-dd"));

        i++;
    }

    // 绘制折线
    painter.setPen(QPen(getColor(6), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(points.data(), points.size());

    // 绘制点和数值
    painter.setBrush(QColor("#2d2d2d"));
    for (int j = 0; j < points.size(); ++j) {
        painter.setPen(QPen(getColor(6), 2));
        painter.drawEllipse(points[j], 3, 3);

        // 数值
        auto it = m_trendData.begin();
        std::advance(it, j);
        if (it.value() > 0) {
            painter.setPen(Qt::white);
            painter.setBackgroundMode(Qt::TransparentMode);
            painter.drawText(points[j].x() - 15, points[j].y() - 20, 30, 15, Qt::AlignCenter, QString::number(it.value()));
        }
    }
}

QColor SimpleChartWidget::getColor(int index) const
{
    static const QStringList palette = {
        "#7696B3", // 钢蓝
        "#D69E68", // 大地橙
        "#7FA882", // 鼠尾草绿
        "#C96A6A", // 柔和砖红
        "#B48EAD", // 莫兰迪紫
        "#8C949E", // 冷灰
        "#88C0D0", // 北欧蓝
        "#EBCB8B", // 麦穗黄
        "#BF616A"  // 浆果红
    };
    return QColor(palette[index % palette.size()]);
}
