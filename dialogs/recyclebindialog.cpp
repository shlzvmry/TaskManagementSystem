#include "recyclebindialog.h"
#include "models/taskmodel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QFrame>

RecycleBinDialog::RecycleBinDialog(QWidget *parent)
    : QDialog(parent)
    , taskModel(nullptr)
{
    setupUI();
    setWindowTitle("回收站");
    setupTable();
}

RecycleBinDialog::~RecycleBinDialog()
{
}

void RecycleBinDialog::setupUI()
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(900, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel *titleLabel = new QLabel("回收站管理", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 顶部工具栏
    QFrame *topFrame = new QFrame(this);
    topFrame->setFrameShape(QFrame::StyledPanel);
    topFrame->setFrameShadow(QFrame::Raised);
    QHBoxLayout *topLayout = new QHBoxLayout(topFrame);
    topLayout->setContentsMargins(10, 10, 10, 10);
    topLayout->setSpacing(10);

    m_statusLabel = new QLabel("共 0 个已删除任务", this);
    topLayout->addWidget(m_statusLabel);
    topLayout->addStretch();

    m_restoreBtn = new QPushButton("恢复选中", this);
    m_restoreBtn->setEnabled(false);
    connect(m_restoreBtn, &QPushButton::clicked, this, &RecycleBinDialog::onRestoreClicked);
    topLayout->addWidget(m_restoreBtn);

    m_deleteBtn = new QPushButton("永久删除", this);
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &RecycleBinDialog::onDeletePermanentlyClicked);
    topLayout->addWidget(m_deleteBtn);

    m_clearBtn = new QPushButton("清空回收站", this);
    m_clearBtn->setEnabled(false);
    connect(m_clearBtn, &QPushButton::clicked, this, &RecycleBinDialog::onClearAllClicked);
    topLayout->addWidget(m_clearBtn);

    QPushButton *refreshBtn = new QPushButton("刷新", this);
    connect(refreshBtn, &QPushButton::clicked, this, &RecycleBinDialog::onRefreshClicked);
    topLayout->addWidget(refreshBtn);

    QPushButton *closeBtn = new QPushButton("关闭", this);
    connect(closeBtn, &QPushButton::clicked, this, &RecycleBinDialog::onCloseClicked);
    topLayout->addWidget(closeBtn);

    mainLayout->addWidget(topFrame);

    // 表格
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSortingEnabled(true);
    m_tableWidget->setShowGrid(true);
    m_tableWidget->setGridStyle(Qt::SolidLine);

    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &RecycleBinDialog::updateButtonStates);

    mainLayout->addWidget(m_tableWidget);
}

void RecycleBinDialog::setupTable()
{
    QStringList headers = {"ID", "标题", "分类", "优先级", "状态", "删除时间", "提醒时间", "创建时间"};
    m_tableWidget->setColumnCount(headers.size());
    m_tableWidget->setHorizontalHeaderLabels(headers);

    QHeaderView *header = m_tableWidget->horizontalHeader();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setMinimumHeight(40);

    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(35);

    m_tableWidget->setColumnWidth(0, 40);
    m_tableWidget->setColumnWidth(1, 200);
    m_tableWidget->setColumnWidth(2, 80);
    m_tableWidget->setColumnWidth(3, 65);
    m_tableWidget->setColumnWidth(4, 65);
    m_tableWidget->setColumnWidth(5, 135);
    m_tableWidget->setColumnWidth(6, 135);
    m_tableWidget->setColumnWidth(7, 135);
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
    if (!taskModel) return;
    deletedTasks = taskModel->getDeletedTasks();
    m_tableWidget->setRowCount(0);

    if (deletedTasks.isEmpty()) {
        m_statusLabel->setText("回收站为空");
        updateButtonStates();
        return;
    }

    m_tableWidget->setRowCount(deletedTasks.size());
    int row = 0;
    for (const QVariantMap &task : deletedTasks) {
        // ... (原有的创建 Item 逻辑，完全相同，只需将 ui->tableWidget 换成 m_tableWidget)
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(task["id"].toInt()));
        idItem->setTextAlignment(Qt::AlignCenter);
        idItem->setData(Qt::UserRole, task["id"]);
        m_tableWidget->setItem(row, 0, idItem);

        // ... (其他列)
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(task["title"].toString()));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(task["category_name"].toString()));

        // 优先级
        int p = task["priority"].toInt();
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(p==0?"紧急":p==1?"重要":p==2?"普通":"不急"));

        // 状态
        int s = task["status"].toInt();
        m_tableWidget->setItem(row, 4, new QTableWidgetItem(s==0?"待办":s==1?"进行中":s==2?"已完成":"已延期"));

        m_tableWidget->setItem(row, 5, new QTableWidgetItem(task["updated_at"].toDateTime().toString("yyyy-MM-dd HH:mm")));
        m_tableWidget->setItem(row, 6, new QTableWidgetItem(task["remind_time"].toDateTime().toString("yyyy-MM-dd HH:mm")));
        m_tableWidget->setItem(row, 7, new QTableWidgetItem(task["created_at"].toDateTime().toString("yyyy-MM-dd HH:mm")));

        row++;
    }
    m_statusLabel->setText(QString("共 %1 个已删除任务").arg(deletedTasks.size()));
    updateButtonStates();
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
    bool hasSelection = m_tableWidget->selectionModel()->hasSelection();
    m_restoreBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_clearBtn->setEnabled(!deletedTasks.isEmpty());
}

int RecycleBinDialog::getSelectedTaskId() const
{
    QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
    if (selectedItems.isEmpty()) return -1;
    int row = selectedItems.first()->row();
    QTableWidgetItem *idItem = m_tableWidget->item(row, 0);
    if (idItem) return idItem->data(Qt::UserRole).toInt();
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
