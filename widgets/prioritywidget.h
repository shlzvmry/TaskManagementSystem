#ifndef PRIORITYWIDGET_H
#define PRIORITYWIDGET_H

#include <QWidget>
#include <QButtonGroup>

class QPushButton;

class PriorityWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PriorityWidget(QWidget *parent = nullptr);

    int getPriority() const;
    void setPriority(int priority);
    QString getPriorityText() const;
    QColor getPriorityColor() const;

    static QMap<int, QString> getPriorityOptions();
    static QString getPriorityText(int priority);
    static QColor getPriorityColor(int priority);

signals:
    void priorityChanged(int priority);

private slots:
    void onButtonClicked(int id);

private:
    QButtonGroup *m_buttonGroup;
    QPushButton *m_buttons[4];

    void setupUI();
    void updateButtonStyles();
};

#endif // PRIORITYWIDGET_H
