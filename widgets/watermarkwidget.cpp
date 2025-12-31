#include "watermarkwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QScreen>
#include <QDebug>

WatermarkWidget::WatermarkWidget(const QString &text, QWidget *parent)
    : QWidget(parent), watermarkText(text)
{
    // 设置透明背景
    setAttribute(Qt::WA_TransparentForMouseEvents);

    // 设置为无边框和透明
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 确保覆盖整个父窗口
    if (parent) {
        setGeometry(0, 0, parent->width(), parent->height());
    }
}

void WatermarkWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont font("Microsoft YaHei", 16, QFont::Light);
    painter.setOpacity(0.15);
    painter.setPen(QColor("#657896"));
    painter.save();
    painter.translate(width() / 2, height() / 2);
    painter.rotate(-30);

    // 计算网格
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(watermarkText);
    int textHeight = fm.height();
    int gridSize = qMax(textWidth, textHeight) + 100; // 网格间距

    // 绘制网格水印
    for (int x = -width(); x < width() * 2; x += gridSize) {
        for (int y = -height(); y < height() * 2; y += gridSize) {
            painter.drawText(x, y, watermarkText);
        }
    }

    painter.restore();

    // 在四个角落也添加小水印
    painter.setOpacity(0.08);
    QFont smallFont("Microsoft YaHei", 10);
    painter.setFont(smallFont);

    painter.drawText(10, 20, watermarkText); // 左上角
    painter.drawText(width() - fm.horizontalAdvance(watermarkText) - 10, 20, watermarkText); // 右上角
    painter.drawText(10, height() - 10, watermarkText); // 左下角
    painter.drawText(width() - fm.horizontalAdvance(watermarkText) - 10, height() - 10, watermarkText); // 右下角
}

void WatermarkWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update(); // 重绘
}
