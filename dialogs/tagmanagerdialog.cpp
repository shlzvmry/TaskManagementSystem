#include "tagmanagerdialog.h"
#include "database/database.h"
#include "widgets/tagwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QHeaderView>

TagManagerDialog::TagManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("标签管理");
    refreshTags();
}

TagManagerDialog::~TagManagerDialog() {}

void TagManagerDialog::setupUI()
{
    resize(805, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QLabel *titleLabel = new QLabel("标签管理", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // 左侧：标签列表
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 10, 0);

    QLabel *lblTags = new QLabel("所有标签", this);
    lblTags->setObjectName("labelTags");
    leftLayout->addWidget(lblTags);

    m_tagListWidget = new QListWidget(this);
    m_tagListWidget->setAlternatingRowColors(true);
    connect(m_tagListWidget, &QListWidget::itemClicked, this, &TagManagerDialog::onTagSelected);
    leftLayout->addWidget(m_tagListWidget);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_tagNameInput = new QLineEdit(this);
    m_tagNameInput->setPlaceholderText("新标签名称(6字以内)");

    QPushButton *addBtn = new QPushButton("添加", this);
    addBtn->setIcon(QIcon(":/icons/add_icon.png"));
    connect(addBtn, &QPushButton::clicked, this, &TagManagerDialog::onAddTagClicked);

    inputLayout->addWidget(m_tagNameInput);
    inputLayout->addWidget(addBtn);
    leftLayout->addLayout(inputLayout);

    QPushButton *delTagBtn = new QPushButton("删除选中标签", this);
    delTagBtn->setObjectName("deleteTagButton");
    delTagBtn->setIcon(QIcon(":/icons/delete_icon.png"));
    connect(delTagBtn, &QPushButton::clicked, this, &TagManagerDialog::onDeleteTagClicked);
    leftLayout->addWidget(delTagBtn);

    // 右侧：关联任务
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(10, 0, 0, 0);

    QLabel *lblTasks = new QLabel("关联的任务", this);
    lblTasks->setObjectName("labelTasks");
    rightLayout->addWidget(lblTasks);

    m_taskTableWidget = new QTableWidget(this);
    m_taskTableWidget->setColumnCount(3);
    m_taskTableWidget->setHorizontalHeaderLabels({"ID", "标题", "分类"});
    m_taskTableWidget->horizontalHeader()->setStretchLastSection(true);
    m_taskTableWidget->verticalHeader()->setVisible(false);
    m_taskTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTableWidget->setAlternatingRowColors(true);
    m_taskTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_taskTableWidget->setColumnWidth(0, 50);
    m_taskTableWidget->setColumnWidth(1, 200);

    rightLayout->addWidget(m_taskTableWidget);

    QPushButton *removeRelationBtn = new QPushButton("解除选中任务的关联", this);
    removeRelationBtn->setIcon(QIcon(":/icons/delete_icon.png"));
    connect(removeRelationBtn, &QPushButton::clicked, this, &TagManagerDialog::onRemoveRelationClicked);
    rightLayout->addWidget(removeRelationBtn);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &TagManagerDialog::accept);
    mainLayout->addWidget(btnBox);
}

void TagManagerDialog::refreshTags()
{
    m_tagListWidget->clear();
    m_taskTableWidget->setRowCount(0);
    allTags = Database::instance().getAllTags();

    for (const QVariantMap &tag : allTags) {
        QListWidgetItem *item = new QListWidgetItem(tag["name"].toString());
        item->setData(Qt::UserRole, tag["id"]);
        item->setData(Qt::UserRole + 1, tag["color"]);

        QPixmap pixmap(16, 16);
        pixmap.fill(QColor(tag["color"].toString()));
        item->setIcon(QIcon(pixmap));

        m_tagListWidget->addItem(item);
    }
}

void TagManagerDialog::onTagSelected(QListWidgetItem *item)
{
    if (!item) return;
    int tagId = item->data(Qt::UserRole).toInt();
    QList<QVariantMap> tasks = Database::instance().getTasksByTagId(tagId);

    m_taskTableWidget->setRowCount(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        const QVariantMap &task = tasks[i];
        m_taskTableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(task["id"].toInt())));
        m_taskTableWidget->setItem(i, 1, new QTableWidgetItem(task["title"].toString()));
        QString cat = task["category_name"].toString();
        m_taskTableWidget->setItem(i, 2, new QTableWidgetItem(cat.isEmpty() ? "未分类" : cat));
    }
}

void TagManagerDialog::onAddTagClicked()
{
    QString name = m_tagNameInput->text().trimmed();
    if (name.isEmpty()) return;
    if (name.length() > 6) {
        QMessageBox::warning(this, "格式错误", "标签名称请限制在6个字以内");
        return;
    }
    QString color = TagWidget::generateColor(name);
    if (Database::instance().addTag(name, color)) {
        m_tagNameInput->clear();
        refreshTags();
    } else {
        QMessageBox::warning(this, "错误", "添加标签失败，可能标签已存在");
    }
}


void TagManagerDialog::onDeleteTagClicked()
{
    QListWidgetItem *item = m_tagListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先选择一个标签");
        return;
    }
    int tagId = item->data(Qt::UserRole).toInt();
    QList<QVariantMap> tasks = Database::instance().getTasksByTagId(tagId);
    if (tasks.isEmpty()) {
        if (QMessageBox::question(this, "确认删除", "确定要删除标签 '" + item->text() + "' 吗？") == QMessageBox::Yes) {
            Database::instance().deleteTag(tagId);
            refreshTags();
        }
    } else {
        if (QMessageBox::warning(this, "关联警告", QString("该标签已关联 %1 个任务，删除将解除关联，继续？").arg(tasks.size()), QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
            Database::instance().deleteTag(tagId);
            refreshTags();
        }
    }
}

void TagManagerDialog::onRemoveRelationClicked()
{
    QListWidgetItem *tagItem = m_tagListWidget->currentItem();
    if (!tagItem) return;
    int currentRow = m_taskTableWidget->currentRow();
    if (currentRow < 0) return;

    int taskId = m_taskTableWidget->item(currentRow, 0)->text().toInt();
    int tagId = tagItem->data(Qt::UserRole).toInt();

    if (Database::instance().removeTaskTagRelation(taskId, tagId)) {
        onTagSelected(tagItem);
    }
}
