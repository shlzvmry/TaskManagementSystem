#include "simplechartwidget.h"
#include "database/database.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QDebug>
#include <QMouseEvent>

SimpleChartWidget::SimpleChartWidget(ChartType type, QString title, QWidget *parent)
    : QWidget(parent), m_type(type), m_title(title), m_isHovering(false)
{
    setMinimumSize(300, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void SimpleChartWidget::setCategoryData(const QMap<QString, int> &data)
{
    m_categoryData = data;
    update();
}

void SimpleChartWidget::setTrendData(const QVector<int> &data, const QStringList &labels, const QStringList &tooltips)
{
    m_trendValues = data;
    m_trendLabels = labels;
    m_tooltips = tooltips;
    update();
}

void SimpleChartWidget::setSubTitle(const QString &subTitle)
{
    m_subTitle = subTitle;
    update();
}

void SimpleChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QString bgMode = Database::instance().getSetting("bg_mode", "dark");
    bool isLight = (bgMode == "light");

    QColor bgColor = isLight ? QColor("#ffffff") : QColor("#262626");
    QColor borderColor = isLight ? QColor("#dcdfe6") : QColor("#3d3d3d");
    QColor textColor = isLight ? QColor("#303133") : QColor("#ffffff");

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect(), 10, 10);

    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);

    painter.setPen(textColor);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(11);
    painter.setFont(font);
    QRect titleRect(0, 10, width(), 30);
    painter.drawText(titleRect, Qt::AlignCenter, m_title);

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
    int pieSize = size * 0.8;
    QRect pieRect(rect.left(), rect.top() + (rect.height() - pieSize)/2, pieSize, pieSize);

    int startAngle = 0;
    int colorIndex = 0;

    for (auto it = m_categoryData.begin(); it != m_categoryData.end(); ++it) {
        if (it.value() == 0) continue;

        int spanAngle = (it.value() * 360 * 16) / total;
        painter.setBrush(getColor(colorIndex));
        painter.setPen(Qt::NoPen);
        painter.drawPie(pieRect, startAngle, spanAngle);

        startAngle += spanAngle;
        colorIndex++;
    }

    int legendX = pieRect.right() + 20;
    int legendY = rect.top() + 20;
    colorIndex = 0;
    QFont legendFont = painter.font();
    legendFont.setPointSize(9);
    legendFont.setBold(false);
    painter.setFont(legendFont);

    // 获取当前模式以调整文字颜色
    QString bgMode = Database::instance().getSetting("bg_mode", "dark");
    QColor legendTextColor = (bgMode == "light") ? QColor("#606266") : QColor("#cccccc");

    for (auto it = m_categoryData.begin(); it != m_categoryData.end(); ++it) {
        if (it.value() == 0) continue;

        painter.setBrush(getColor(colorIndex));
        painter.setPen(Qt::NoPen);
        painter.drawRect(legendX, legendY, 12, 12);

        painter.setPen(legendTextColor);
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

    // 获取当前模式以调整文字颜色
    QString bgMode = Database::instance().getSetting("bg_mode", "dark");
    QColor labelColor = (bgMode == "light") ? QColor("#606266") : QColor("#cccccc");

    for (auto it = m_categoryData.begin(); it != m_categoryData.end(); ++it) {
        int barHeight = (int)((double)it.value() / maxVal * (rect.height() - 20));

        QRect barRect(x, rect.bottom() - barHeight, barWidth, barHeight);

        painter.setBrush(getColor(colorIndex));
        painter.setPen(Qt::NoPen);
        painter.drawRect(barRect);

        painter.setPen(labelColor); // 柱状图上方数字颜色跟随标签颜色，或者固定白色
        if (barHeight > 20) {
            painter.setPen(Qt::white); // 如果柱子够高，数字画在柱子里，用白色
            painter.drawText(barRect.adjusted(0, 0, 0, 0), Qt::AlignCenter, QString::number(it.value()));
        } else {
            painter.setPen(labelColor); // 柱子太矮，画在上面
            painter.drawText(barRect.adjusted(0, -20, 0, 0), Qt::AlignCenter | Qt::AlignBottom, QString::number(it.value()));
        }

        painter.setPen(labelColor);
        QRect labelRect(x - 5, rect.bottom() + 10, barWidth + 10, 20);
        painter.drawText(labelRect, Qt::AlignCenter, it.key());

        x += barWidth + spacing;
        colorIndex++;
    }
}

void SimpleChartWidget::drawLineChart(QPainter &painter, const QRect &rect)
{
    // 获取当前文本颜色（从 paintEvent 传下来的 painter 状态）
    QColor axisColor = painter.pen().color();
    // 稍微变淡一点作为坐标轴颜色
    axisColor.setAlpha(100);

    if (!m_subTitle.isEmpty()) {
        painter.setPen(axisColor); // 使用自适应颜色
        QFont subFont = painter.font();
        subFont.setPointSize(9);
        subFont.setBold(false);
        painter.setFont(subFont);
        painter.drawText(rect.left(), rect.top() - 15, m_subTitle);
    }

    if (m_trendValues.isEmpty()) return;

    m_currentPoints.clear();

    int maxVal = 0;
    for (int val : m_trendValues) if (val > maxVal) maxVal = val;
    maxVal = maxVal < 5 ? 5 : maxVal + 1;

    // 绘制坐标轴
    painter.setPen(axisColor); // 使用自适应颜色
    painter.drawLine(rect.bottomLeft(), rect.bottomRight());
    painter.drawLine(rect.bottomLeft(), rect.topLeft());

    int count = m_trendValues.size();
    double stepX = count > 1 ? (double)rect.width() / (count - 1) : 0;
    bool showXLabels = (count <= 31);

    QVector<QPointF> points;
    for (int i = 0; i < count; ++i) {
        double x = rect.left() + i * stepX;
        double y = rect.bottom() - ((double)m_trendValues[i] / maxVal * rect.height());
        QPointF pt(x, y);
        points.append(pt);
        m_currentPoints.append(pt);

        if (showXLabels && i < m_trendLabels.size()) {
            painter.setPen(axisColor); // 使用自适应颜色
            QFont f = painter.font();
            f.setPointSize(8);
            painter.setFont(f);

            if (count > 15 && (i % 2 != 0)) {
            } else {
                painter.drawText(x - 20, rect.bottom() + 5, 40, 20, Qt::AlignCenter, m_trendLabels.at(i));
            }
        }
    }

    // 绘制折线
    painter.setRenderHint(QPainter::Antialiasing);
    // 获取主题色作为折线颜色 (第6个颜色是灰色，这里我们固定用主题色或者某个显眼颜色)
    painter.setPen(QPen(getColor(6), 2));
    if (points.size() > 1) painter.drawPolyline(points.data(), points.size());

    // 鼠标悬停处理
    int closestIndex = -1;
    double minDist = 10000.0;

    if (m_isHovering) {
        for (int i = 0; i < m_currentPoints.size(); ++i) {
            double dist = std::abs(m_currentPoints[i].x() - m_mousePos.x());
            if (dist < minDist && dist < stepX / 2 + 5) {
                minDist = dist;
                closestIndex = i;
            }
        }
    }

    for (int j = 0; j < points.size(); ++j) {
        // 点的填充色：跟随背景色
        QString bgMode = Database::instance().getSetting("bg_mode", "dark");
        painter.setBrush(bgMode == "light" ? Qt::white : QColor("#2d2d2d"));

        if (j == closestIndex) {
            painter.setPen(QPen(axisColor, 2)); // 高亮圈颜色
            painter.drawEllipse(points[j], 5, 5);

            QString tipText = QString("数值: %1").arg(m_trendValues[j]);
            if (j < m_tooltips.size()) {
                tipText = QString("%1\n%2").arg(m_tooltips[j]).arg(tipText);
            } else if (j < m_trendLabels.size()) {
                tipText = QString("%1\n%2").arg(m_trendLabels[j]).arg(tipText);
            }

            QFont tipFont = painter.font();
            tipFont.setPointSize(9);
            QFontMetrics fm(tipFont);
            QRect tipRect = fm.boundingRect(QRect(0,0,0,0), Qt::AlignLeft, tipText);
            tipRect.adjust(-5, -5, 5, 5);
            tipRect.moveCenter(points[j].toPoint() + QPoint(0, -35));

            if (tipRect.left() < 0) tipRect.moveLeft(5);
            if (tipRect.right() > width()) tipRect.moveRight(width() - 5);

            // Tooltip 背景：半透明黑
            painter.setBrush(QColor(0, 0, 0, 200));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(tipRect, 4, 4);

            painter.setPen(Qt::white);
            painter.setFont(tipFont);
            painter.drawText(tipRect, Qt::AlignCenter, tipText);

        } else {
            painter.setPen(QPen(getColor(6), 2));
            painter.drawEllipse(points[j], 3, 3);

            // 数值显示
            if (count <= 15 && m_trendValues[j] > 0) {
                painter.setPen(axisColor); // 使用自适应颜色
                painter.drawText(points[j].x() - 15, points[j].y() - 20, 30, 15, Qt::AlignCenter, QString::number(m_trendValues[j]));
            }
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

void SimpleChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();
    m_isHovering = true;
    update();
    QWidget::mouseMoveEvent(event);
}

void SimpleChartWidget::leaveEvent(QEvent *event)
{
    m_isHovering = false;
    update();
    QWidget::leaveEvent(event);
}
