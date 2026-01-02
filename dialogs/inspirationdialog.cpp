#include "inspirationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>

InspirationDialog::InspirationDialog(QWidget *parent)
    : QDialog(parent), m_id(-1)
{
    setupUI();
    setWindowTitle("记录灵感");
}

InspirationDialog::InspirationDialog(const QVariantMap &data, QWidget *parent)
    : QDialog(parent), m_id(data.value("id", -1).toInt())
{
    setupUI();
    setWindowTitle("编辑灵感");
    populateData(data);
}

void InspirationDialog::setupUI()
{
    resize(400, 280);

    // 去掉问号按钮，保持简洁
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    // 标题提示
    QLabel *tipLabel = new QLabel("捕捉当下的想法...", this);
    tipLabel->setStyleSheet("color: #888888; font-style: italic;");
    layout->addWidget(tipLabel);

    // 内容输入 (记事贴风格)
    m_contentEdit = new QTextEdit(this);
    m_contentEdit->setPlaceholderText("在这里输入灵感内容...");
    m_contentEdit->setObjectName("inspirationContentEdit");
    layout->addWidget(m_contentEdit, 1); // 占据主要空间

    // 标签输入
    QHBoxLayout *tagLayout = new QHBoxLayout();
    QLabel *tagIcon = new QLabel("🏷️", this);
    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText("标签 (如: 创意, 待办)");
    m_tagsEdit->setObjectName("inspirationTagEdit");

    tagLayout->addWidget(tagIcon);
    tagLayout->addWidget(m_tagsEdit);
    layout->addLayout(tagLayout);

    // 按钮栏
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(); // 弹簧，将按钮推到右边

    QPushButton *cancelBtn = new QPushButton("取消", this);
    QPushButton *saveBtn = new QPushButton("保存", this);
    saveBtn->setObjectName("saveInspirationBtn"); // 用于样式定制
    saveBtn->setDefault(true); // 回车默认触发

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &InspirationDialog::onSave);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);
}

void InspirationDialog::populateData(const QVariantMap &data)
{
    m_contentEdit->setText(data.value("content").toString());
    m_tagsEdit->setText(data.value("tags").toString());
}

QVariantMap InspirationDialog::getData() const
{
    QVariantMap data;
    if (m_id != -1) data["id"] = m_id;
    data["content"] = m_contentEdit->toPlainText().trimmed();
    data["tags"] = m_tagsEdit->text().trimmed();
    return data;
}

void InspirationDialog::onSave()
{
    if (m_contentEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "内容不能为空");
        return;
    }
    accept();
}
