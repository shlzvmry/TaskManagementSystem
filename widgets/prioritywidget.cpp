#include "prioritywidget.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QDebug>
#include <QColor>

PriorityWidget::PriorityWidget(QWidget *parent)
    : QWidget(parent)
    , m_buttonGroup(new QButtonGroup(this))
{
    setupUI();
}

void PriorityWidget::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    QStringList priorityTexts = {"紧急", "重要", "普通", "不急"};

    for (int i = 0; i < 4; ++i) {
        QPushButton *button = new QPushButton(priorityTexts[i], this);
        button->setCheckable(true);
        button->setFixedSize(70, 32);
        button->setObjectName(QString("priorityBtn_%1").arg(i));

        m_buttons[i] = button;
        m_buttonGroup->addButton(button, i);
        layout->addWidget(button);
    }

    m_buttons[2]->setChecked(true);

    connect(m_buttonGroup, &QButtonGroup::idClicked,
            this, &PriorityWidget::onButtonClicked);
}

int PriorityWidget::getPriority() const
{
    return m_buttonGroup->checkedId();
}

void PriorityWidget::setPriority(int priority)
{
    if (priority >= 0 && priority < 4) {
        m_buttons[priority]->setChecked(true);
    }
}

QString PriorityWidget::getPriorityText() const
{
    return getPriorityText(getPriority());
}

QColor PriorityWidget::getPriorityColor() const
{
    return getPriorityColor(getPriority());
}

void PriorityWidget::onButtonClicked(int id)
{
    emit priorityChanged(id);
}

QMap<int, QString> PriorityWidget::getPriorityOptions()
{
    return {
        {0, "紧急"},
        {1, "重要"},
        {2, "普通"},
        {3, "不急"}
    };
}

QString PriorityWidget::getPriorityText(int priority)
{
    switch (priority) {
    case 0: return "紧急";
    case 1: return "重要";
    case 2: return "普通";
    case 3: return "不急";
    default: return "未知";
    }
}

QColor PriorityWidget::getPriorityColor(int priority)
{
    switch (priority) {
    case 0: return QColor("#FF4444");
    case 1: return QColor("#FF9900");
    case 2: return QColor("#4CAF50");
    case 3: return QColor("#9E9E9E");
    default: return QColor("#657896");
    }
}
