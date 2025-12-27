#include "tagmanagerdialog.h"
#include "ui_tagmanagerdialog.h"
#include "database/database.h"
#include "widgets/tagwidget.h"
#include <QMessageBox>
#include <QDebug>

TagManagerDialog::TagManagerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TagManagerDialog)
{
    ui->setupUi(this);
    setupUI();
    setupConnections();
    refreshTags();
}

TagManagerDialog::~TagManagerDialog()
{
    delete ui;
}

void TagManagerDialog::setupUI()
{
    // 设置分割器比例
    ui->splitter->setStretchFactor(0, 12);
    ui->splitter->setStretchFactor(1, 23);

    // 设置表格列宽
    ui->taskTableWidget->setColumnWidth(0, 50);  // ID
    ui->taskTableWidget->setColumnWidth(1, 280); // 标题
}

void TagManagerDialog::setupConnections()
{
    connect(ui->tagListWidget, &QListWidget::itemClicked, this, &TagManagerDialog::onTagSelected);
    connect(ui->addTagButton, &QPushButton::clicked, this, &TagManagerDialog::onAddTagClicked);
    connect(ui->deleteTagButton, &QPushButton::clicked, this, &TagManagerDialog::onDeleteTagClicked);
    connect(ui->removeRelationButton, &QPushButton::clicked, this, &TagManagerDialog::onRemoveRelationClicked);
}

void TagManagerDialog::refreshTags()
{
    ui->tagListWidget->clear();
    ui->taskTableWidget->setRowCount(0);
    allTags = Database::instance().getAllTags();

    for (const QVariantMap &tag : allTags) {
        QListWidgetItem *item = new QListWidgetItem(tag["name"].toString());
        item->setData(Qt::UserRole, tag["id"]);
        item->setData(Qt::UserRole + 1, tag["color"]);

        // 设置图标颜色
        QPixmap pixmap(16, 16);
        pixmap.fill(QColor(tag["color"].toString()));
        item->setIcon(QIcon(pixmap));

        ui->tagListWidget->addItem(item);
    }
}

void TagManagerDialog::onTagSelected(QListWidgetItem *item)
{
    if (!item) return;

    int tagId = item->data(Qt::UserRole).toInt();
    QList<QVariantMap> tasks = Database::instance().getTasksByTagId(tagId);

    ui->taskTableWidget->setRowCount(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        const QVariantMap &task = tasks[i];

        // ID列
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(task["id"].toInt()));
        idItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTableWidget->setItem(i, 0, idItem);

        // 标题列
        QString title = task["title"].toString();
        QTableWidgetItem *titleItem = new QTableWidgetItem(title);
        titleItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTableWidget->setItem(i, 1, titleItem);

        // 分类列
        QString categoryName = task["category_name"].toString();
        QTableWidgetItem *categoryItem = new QTableWidgetItem(categoryName.isEmpty() ? "未分类" : categoryName);
        categoryItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTableWidget->setItem(i, 2, categoryItem);
    }
}

void TagManagerDialog::onAddTagClicked()
{
    QString name = ui->tagNameInput->text().trimmed();
    if (name.isEmpty()) return;

    // 限制标签长度
    if (name.length() > 6) {
        QMessageBox::warning(this, "格式错误", "标签名称请限制在6个字以内");
        return;
    }

    QString color = TagWidget::generateColor(name);
    if (Database::instance().addTag(name, color)) {
        ui->tagNameInput->clear();
        refreshTags();
    } else {
        QMessageBox::warning(this, "错误", "添加标签失败，可能标签已存在");
    }
}

void TagManagerDialog::onDeleteTagClicked()
{
    QListWidgetItem *item = ui->tagListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先选择一个标签");
        return;
    }

    int tagId = item->data(Qt::UserRole).toInt();
    QString tagName = item->text();

    // 检查关联任务
    QList<QVariantMap> tasks = Database::instance().getTasksByTagId(tagId);

    if (tasks.isEmpty()) {
        // 无关联，直接删除
        if (QMessageBox::question(this, "确认删除", "确定要删除标签 '" + tagName + "' 吗？") == QMessageBox::Yes) {
            Database::instance().deleteTag(tagId);
            refreshTags();
        }
    } else {
        // 有关联，显示警告
        QString msg = QString("标签 '%1' 已关联 %2 个任务：\n\n").arg(tagName).arg(tasks.size());
        int count = 0;
        for (const QVariantMap &task : tasks) {
            if (count++ >= 5) {
                msg += "...等";
                break;
            }
            msg += QString("- [%1] %2\n").arg(task["id"].toString()).arg(task["title"].toString());
        }
        msg += "\n删除标签将自动解除这些关联。确定要继续吗？";

        if (QMessageBox::warning(this, "关联警告", msg, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            Database::instance().deleteTag(tagId);
            refreshTags();
        }
    }
}

void TagManagerDialog::onRemoveRelationClicked()
{
    // 1. 获取当前选中的标签
    QListWidgetItem *tagItem = ui->tagListWidget->currentItem();
    if (!tagItem) {
        QMessageBox::warning(this, "提示", "请先在左侧选择一个标签");
        return;
    }
    int tagId = tagItem->data(Qt::UserRole).toInt();

    // 2. 获取当前选中的任务
    int currentRow = ui->taskTableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请在右侧列表中选择要解除关联的任务");
        return;
    }

    // 获取任务ID (第0列)
    QTableWidgetItem *idItem = ui->taskTableWidget->item(currentRow, 0);
    if (!idItem) return;
    int taskId = idItem->text().toInt();
    QString taskTitle = ui->taskTableWidget->item(currentRow, 1)->text();

    // 3. 确认并执行
    QString msg = QString("确定要移除任务 '%1' 的 '%2' 标签吗？").arg(taskTitle).arg(tagItem->text());
    if (QMessageBox::question(this, "确认解除", msg) == QMessageBox::Yes) {
        if (Database::instance().removeTaskTagRelation(taskId, tagId)) {
            // 刷新右侧任务列表
            onTagSelected(tagItem);
        } else {
            QMessageBox::warning(this, "错误", "解除关联失败");
        }
    }
}
