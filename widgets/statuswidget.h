#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>
#include <QButtonGroup>

class QPushButton;

class StatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatusWidget(QWidget *parent = nullptr);

    int getStatus() const;
    void setStatus(int status);
    QString getStatusText() const;
    QColor getStatusColor() const;

    static QMap<int, QString> getStatusOptions();
    static QString getStatusText(int status);
    static QColor getStatusColor(int status);

signals:
    void statusChanged(int status);

private slots:
    void onButtonClicked(int id);

private:
    QButtonGroup *m_buttonGroup;
    QPushButton *m_buttons[4];

    void setupUI();
};

#endif // STATUSWIDGET_H
