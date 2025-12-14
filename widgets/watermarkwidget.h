#ifndef WATERMARKWIDGET_H
#define WATERMARKWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

class WatermarkWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WatermarkWidget(const QString &text, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString watermarkText;
};

#endif // WATERMARKWIDGET_H
