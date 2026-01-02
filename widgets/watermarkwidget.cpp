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
    setAttribute(Qt::WA_TransparentForMouseEvents);

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

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

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(watermarkText);
    int textHeight = fm.height();
    int gridSize = qMax(textWidth, textHeight) + 100;

    for (int x = -width(); x < width() * 2; x += gridSize) {
        for (int y = -height(); y < height() * 2; y += gridSize) {
            painter.drawText(x, y, watermarkText);
        }
    }

    painter.restore();
    painter.setOpacity(0.08);
    QFont smallFont("Microsoft YaHei", 10);
    painter.setFont(smallFont);

    painter.drawText(10, 20, watermarkText);
    painter.drawText(width() - fm.horizontalAdvance(watermarkText) - 10, 20, watermarkText);
    painter.drawText(10, height() - 10, watermarkText);
    painter.drawText(width() - fm.horizontalAdvance(watermarkText) - 10, height() - 10, watermarkText);
}

void WatermarkWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
