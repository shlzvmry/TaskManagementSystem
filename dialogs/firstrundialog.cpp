#include "firstrundialog.h"
#include "database/database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QButtonGroup>

FirstRunDialog::FirstRunDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("欢迎使用 - 初始设置");
    resize(400, 500);
    setupUI();
}

void FirstRunDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("👋 欢迎！请选择您的身份以初始化任务分类：", this);
    title->setStyleSheet("font-size: 14px; font-weight: bold; color: #657896;");
    title->setWordWrap(true);
    layout->addWidget(title);

    m_studentBtn = new QRadioButton("我是学生 (包含作业、复习、考试等)", this);
    m_workerBtn = new QRadioButton("我是职工 (包含工作、汇报、会议等)", this);
    m_customBtn = new QRadioButton("自定义 (空白模板)", this);

    m_studentBtn->setChecked(true);

    QButtonGroup *group = new QButtonGroup(this);
    group->addButton(m_studentBtn);
    group->addButton(m_workerBtn);
    group->addButton(m_customBtn);

    layout->addWidget(m_studentBtn);
    layout->addWidget(m_workerBtn);
    layout->addWidget(m_customBtn);

    connect(m_studentBtn, &QRadioButton::toggled, this, &FirstRunDialog::onTypeChanged);
    connect(m_workerBtn, &QRadioButton::toggled, this, &FirstRunDialog::onTypeChanged);
    connect(m_customBtn, &QRadioButton::toggled, this, &FirstRunDialog::onTypeChanged);

    layout->addWidget(new QLabel("预设分类预览 (您稍后可以在设置中修改)：", this));

    m_categoryList = new QListWidget(this);
    layout->addWidget(m_categoryList);

    QPushButton *okBtn = new QPushButton("开始使用", this);
    okBtn->setFixedHeight(40);
    okBtn->setStyleSheet("background-color: #657896; color: white; font-weight: bold; border-radius: 4px;");
    connect(okBtn, &QPushButton::clicked, this, &FirstRunDialog::onConfirm);

    layout->addStretch();
    layout->addWidget(okBtn);

    onTypeChanged(); // 加载初始状态
}

void FirstRunDialog::onTypeChanged()
{
    m_categoryList->clear();
    QStringList cats, colors;

    if (m_studentBtn->isChecked()) {
        cats << "作业" << "物资增添" << "个人生活" << "考试" << "复习安排" << "工作";
        colors << "#FF6B6B" << "#4ECDC4" << "#45B7D1" << "#96CEB4" << "#FFEAA7" << "#DDA0DD";
    } else if (m_workerBtn->isChecked()) {
        cats << "工作任务" << "汇报总结" << "会议" << "个人生活" << "物资采购" << "学习提升";
        colors << "#7696B3" << "#D69E68" << "#7FA882" << "#88C0D0" << "#B48EAD" << "#EBCB8B";
    }

    loadDefaults(cats, colors);
}

void FirstRunDialog::loadDefaults(const QStringList &categories, const QStringList &colors)
{
    for(int i=0; i<categories.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(categories[i]);
        QPixmap pix(16, 16);
        pix.fill(QColor(i < colors.size() ? colors[i] : "#657896"));
        item->setIcon(QIcon(pix));
        m_categoryList->addItem(item);
    }
}

void FirstRunDialog::onConfirm()
{
    Database::instance().clearCategories();

    for(int i=0; i<m_categoryList->count(); ++i) {
        // 简单处理颜色生成
        QString name = m_categoryList->item(i)->text();
        QString color = "#657896";
        if (m_studentBtn->isChecked()) {
            QStringList colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7", "#DDA0DD"};
            if(i < colors.size()) color = colors[i];
        } else if (m_workerBtn->isChecked()) {
            QStringList colors = {"#7696B3", "#D69E68", "#7FA882", "#88C0D0", "#B48EAD", "#EBCB8B"};
            if(i < colors.size()) color = colors[i];
        }

        Database::instance().addCategory(name, color);
    }

    Database::instance().setSetting("first_run", "false");
    accept();
}
