#include "recyclebindialog.h"
#include "ui_recyclebindialog.h"
#include "models/taskmodel.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QDebug>

RecycleBinDialog::RecycleBinDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RecycleBinDialog)
    , taskModel(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("回收站");
    setupUI();
    setupConnections();
    setupTable();
}

RecycleBinDialog::~RecycleBinDialog()
{
    delete ui;
}

void RecycleBinDialog::setupUI()
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(900, 600);

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSortingEnabled(true);
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->setGridStyle(Qt::SolidLine);

    QHeaderView *horizontalHeader = ui->tableWidget->horizontalHeader();
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader->setMinimumHeight(40);

    ui->tableWidget->setColumnCount(8);

    ui->tableWidget->setColumnWidth(0, 40);
    ui->tableWidget->setColumnWidth(1, 200);
    ui->tableWidget->setColumnWidth(2, 80);
    ui->tableWidget->setColumnWidth(3, 65);
    ui->tableWidget->setColumnWidth(4, 65);
    ui->tableWidget->setColumnWidth(5, 130);
    ui->tableWidget->setColumnWidth(6, 130);
    ui->tableWidget->setColumnWidth(7, 130);

    QStringList headers = {"ID", "标题", "分类", "优先级", "状态", "删除时间", "提醒时间", "创建时间"};
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(35);
    ui->tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void RecycleBinDialog::setupConnections()
{
    connect(ui->restoreButton, &QPushButton::clicked, this, &RecycleBinDialog::onRestoreClicked);
    connect(ui->deletePermanentlyButton, &QPushButton::clicked, this, &RecycleBinDialog::onDeletePermanentlyClicked);
    connect(ui->clearAllButton, &QPushButton::clicked, this, &RecycleBinDialog::onClearAllClicked);
    connect(ui->refreshButton, &QPushButton::clicked, this, &RecycleBinDialog::onRefreshClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &RecycleBinDialog::onCloseClicked);

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged,
            this, &RecycleBinDialog::updateButtonStates);
}

void RecycleBinDialog::setupTable()
{
    ui->tableWidget->setRowCount(0);
}

void RecycleBinDialog::setTaskModel(TaskModel *model)
{
    taskModel = model;
    if (taskModel) {
        refreshDeletedTasks();

        connect(taskModel, &TaskModel::taskRestored, this, [this](int taskId) {
            Q_UNUSED(taskId);
            refreshDeletedTasks();
        });

        connect(taskModel, &TaskModel::taskPermanentlyDeleted, this, [this](int taskId) {
            Q_UNUSED(taskId);
            refreshDeletedTasks();
        });
    }
}

void RecycleBinDialog::refreshDeletedTasks()
{
    qDebug() << "开始刷新回收站数据...";

    if (!taskModel) {
        qDebug() << "错误: taskModel 为空";
        ui->statusLabel->setText("错误: 任务模型未初始化");
        return;
    }

    deletedTasks = taskModel->getDeletedTasks();
    qDebug() << "获取到" << deletedTasks.size() << "个已删除任务";

    ui->tableWidget->setRowCount(0);

    if (deletedTasks.isEmpty()) {
        ui->tableWidget->setRowCount(0);
        ui->statusLabel->setText("回收站为空");
        updateButtonStates();
        return;
    }

    ui->tableWidget->setRowCount(deletedTasks.size());

    int row = 0;
    for (const QVariantMap &task : deletedTasks) {
        qDebug() << "处理第" << row << "行: ID=" << task["id"].toInt()
            << ", 标题=" << task["title"].toString();

        try {
            QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(task["id"].toInt()));
            idItem->setTextAlignment(Qt::AlignCenter);
            idItem->setData(Qt::UserRole, task["id"]);
            ui->tableWidget->setItem(row, 0, idItem);

            QString title = task["title"].toString();
            QTableWidgetItem *titleItem = new QTableWidgetItem(title);
            QString description = task["description"].toString();
            if (!description.isEmpty()) {
                titleItem->setToolTip(description);
            }
            titleItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 1, titleItem);

            QString categoryName = task["category_name"].toString();
            QTableWidgetItem *categoryItem = new QTableWidgetItem(categoryName.isEmpty() ? "未分类" : categoryName);
            categoryItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 2, categoryItem);

            int priority = task["priority"].toInt();
            QString priorityText;
            switch (priority) {
            case 0: priorityText = "紧急"; break;
            case 1: priorityText = "重要"; break;
            case 2: priorityText = "普通"; break;
            case 3: priorityText = "不急"; break;
            default: priorityText = "未知";
            }
            QTableWidgetItem *priorityItem = new QTableWidgetItem(priorityText);
            priorityItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 3, priorityItem);

            int status = task["status"].toInt();
            QString statusText;
            switch (status) {
            case 0: statusText = "待办"; break;
            case 1: statusText = "进行中"; break;
            case 2: statusText = "已完成"; break;
            case 3: statusText = "已延期"; break;
            default: statusText = "未知";
            }
            QTableWidgetItem *statusItem = new QTableWidgetItem(statusText);
            statusItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 4, statusItem);

            QDateTime deletedTime = task["updated_at"].toDateTime();
            QTableWidgetItem *deletedItem = new QTableWidgetItem(
                deletedTime.isValid() ? deletedTime.toString("yyyy-MM-dd HH:mm") : "未知时间");
            deletedItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 5, deletedItem);

            QDateTime remindTime = task["remind_time"].toDateTime();
            QTableWidgetItem *remindItem = new QTableWidgetItem(
                remindTime.isValid() ? remindTime.toString("yyyy-MM-dd HH:mm") : "-");
            remindItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 6, remindItem);

            QDateTime createdTime = task["created_at"].toDateTime();
            QTableWidgetItem *createdItem = new QTableWidgetItem(
                createdTime.isValid() ? createdTime.toString("yyyy-MM-dd HH:mm") : "未知时间");
            createdItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->setItem(row, 7, createdItem);

            row++;

        } catch (const std::exception &e) {
            qDebug() << "处理任务数据时出错:" << e.what();
            continue;
        }
    }

    ui->statusLabel->setText(QString("共 %1 个已删除任务").arg(deletedTasks.size()));

    updateButtonStates();

    ui->tableWidget->viewport()->update();
    qDebug() << "回收站刷新完成，显示" << row << "行数据";
}

void RecycleBinDialog::onRestoreClicked()
{
    int taskId = getSelectedTaskId();
    if (taskId == -1) return;

    showConfirmationDialog("恢复任务", "确定要恢复选中的任务吗？", [this, taskId]() {
        if (taskModel && taskModel->restoreTask(taskId)) {
            QMessageBox::information(this, "成功", "任务恢复成功！");
        } else {
            QMessageBox::warning(this, "错误", "任务恢复失败");
        }
    });
}

void RecycleBinDialog::onDeletePermanentlyClicked()
{
    int taskId = getSelectedTaskId();
    if (taskId == -1) return;

    QString taskTitle;
    for (const QVariantMap &task : deletedTasks) {
        if (task["id"].toInt() == taskId) {
            taskTitle = task["title"].toString();
            break;
        }
    }

    showConfirmationDialog("永久删除",
                           QString("确定要永久删除任务 '%1' 吗？\n此操作不可撤销！").arg(taskTitle),
                           [this, taskId]() {
                               if (taskModel && taskModel->permanentDeleteTask(taskId)) {
                                   QMessageBox::information(this, "成功", "任务已永久删除！");
                               } else {
                                   QMessageBox::warning(this, "错误", "删除失败");
                               }
                           });
}

void RecycleBinDialog::onClearAllClicked()
{
    if (deletedTasks.isEmpty()) return;

    showConfirmationDialog("清空回收站",
                           QString("确定要清空回收站吗？\n这将永久删除 %1 个任务，此操作不可撤销！").arg(deletedTasks.size()),
                           [this]() {
                               int successCount = 0;
                               int failCount = 0;

                               for (const QVariantMap &task : deletedTasks) {
                                   int taskId = task["id"].toInt();
                                   if (taskModel && taskModel->permanentDeleteTask(taskId)) {
                                       successCount++;
                                   } else {
                                       failCount++;
                                   }
                               }

                               if (failCount == 0) {
                                   QMessageBox::information(this, "成功",
                                                            QString("已成功删除 %1 个任务").arg(successCount));
                               } else {
                                   QMessageBox::warning(this, "完成",
                                                        QString("删除完成：成功 %1 个，失败 %2 个").arg(successCount).arg(failCount));
                               }
                           });
}

void RecycleBinDialog::onRefreshClicked()
{
    refreshDeletedTasks();
}

void RecycleBinDialog::onCloseClicked()
{
    accept();
}

void RecycleBinDialog::updateButtonStates()
{
    bool hasSelection = ui->tableWidget->selectionModel()->hasSelection();
    ui->restoreButton->setEnabled(hasSelection);
    ui->deletePermanentlyButton->setEnabled(hasSelection);
    ui->clearAllButton->setEnabled(!deletedTasks.isEmpty());
}

int RecycleBinDialog::getSelectedTaskId() const
{
    QList<QTableWidgetItem*> selectedItems = ui->tableWidget->selectedItems();
    if (selectedItems.isEmpty()) return -1;

    int row = selectedItems.first()->row();
    QTableWidgetItem *idItem = ui->tableWidget->item(row, 0);
    if (idItem) {
        return idItem->data(Qt::UserRole).toInt();
    }

    return -1;
}

void RecycleBinDialog::showConfirmationDialog(const QString &title, const QString &message,
                                              std::function<void()> onConfirmed)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        onConfirmed();
    }
}
