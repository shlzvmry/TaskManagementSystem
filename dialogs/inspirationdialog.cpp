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

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    QLabel *tipLabel = new QLabel("捕捉当下的想法...", this);
    tipLabel->setStyleSheet("color: #888888; font-style: italic;");
    layout->addWidget(tipLabel);

    m_contentEdit = new QTextEdit(this);
    m_contentEdit->setPlaceholderText("在这里输入灵感内容...");
    m_contentEdit->setObjectName("inspirationContentEdit");
    layout->addWidget(m_contentEdit, 1);

    QHBoxLayout *tagLayout = new QHBoxLayout();
    QLabel *tagIcon = new QLabel("🏷️", this);
    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText("标签(每个标签最长6字，逗号分隔)");
    m_tagsEdit->setObjectName("inspirationTagEdit");

    tagLayout->addWidget(tagIcon);
    tagLayout->addWidget(m_tagsEdit);
    layout->addLayout(tagLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消", this);
    QPushButton *saveBtn = new QPushButton("保存", this);
    saveBtn->setObjectName("saveInspirationBtn");
    saveBtn->setDefault(true);

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
    if (m_id != -1) {
        data["id"] = m_id;
    }
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

    QString tagsText = m_tagsEdit->text();
    tagsText.replace("，", ",");

    QStringList rawTags = tagsText.split(",", Qt::SkipEmptyParts);
    QStringList validTags;
    QSet<QString> seenTags;

    for (const QString &t : rawTags) {
        QString tag = t.trimmed();
        if (tag.isEmpty()) continue;

        if (tag.length() > 6) {
            QMessageBox::warning(this, "格式错误", QString("标签 '%1' 超过6个字限制").arg(tag));
            return;
        }

        if (!seenTags.contains(tag)) {
            validTags.append(tag);
            seenTags.insert(tag);
        }
    }

    m_tagsEdit->setText(validTags.join(","));

    accept();
}
