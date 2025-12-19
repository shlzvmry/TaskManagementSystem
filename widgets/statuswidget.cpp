#include "statuswidget.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QDebug>
#include <QColor>

StatusWidget::StatusWidget(QWidget *parent)
    : QWidget(parent)
    , m_buttonGroup(new QButtonGroup(this))
{
    setupUI();
}

void StatusWidget::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    // 创建四个状态按钮
    QStringList statusTexts = {"待办", "进行中", "已完成", "已延期"};

    for (int i = 0; i < 4; ++i) {
        QPushButton *button = new QPushButton(statusTexts[i], this);
        button->setCheckable(true);
        button->setFixedSize(70, 30);
        button->setProperty("status", i);
        button->setObjectName(QString("statusBtn_%1").arg(i));

        m_buttons[i] = button;
        m_buttonGroup->addButton(button, i);
        layout->addWidget(button);
    }

    // 默认选择"待办"状态
    m_buttons[0]->setChecked(true);

    // 连接信号
    connect(m_buttonGroup, &QButtonGroup::idClicked,
            this, &StatusWidget::onButtonClicked);
}

int StatusWidget::getStatus() const
{
    return m_buttonGroup->checkedId();
}

void StatusWidget::setStatus(int status)
{
    if (status >= 0 && status < 4) {
        m_buttons[status]->setChecked(true);
    }
}

QString StatusWidget::getStatusText() const
{
    return getStatusText(getStatus());
}

QColor StatusWidget::getStatusColor() const
{
    return getStatusColor(getStatus());
}

void StatusWidget::onButtonClicked(int id)
{
    emit statusChanged(id);
}

QMap<int, QString> StatusWidget::getStatusOptions()
{
    return {
        {0, "待办"},
        {1, "进行中"},
        {2, "已完成"},
        {3, "已延期"}
    };
}

QString StatusWidget::getStatusText(int status)
{
    switch (status) {
    case 0: return "待办";
    case 1: return "进行中";
    case 2: return "已完成";
    case 3: return "已延期";
    default: return "未知";
    }
}

QColor StatusWidget::getStatusColor(int status)
{
    switch (status) {
    case 0: return QColor("#2196F3"); // 蓝色
    case 1: return QColor("#FF9800"); // 橙色
    case 2: return QColor("#4CAF50"); // 绿色
    case 3: return QColor("#F44336"); // 红色
    default: return QColor("#657896"); // 主题色
    }
}
