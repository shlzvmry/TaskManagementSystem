#include "inspirationrecyclebindialog.h"
#include "models/inspirationmodel.h"

InspirationRecycleBinDialog::InspirationRecycleBinDialog(InspirationModel *model, QWidget *parent)
    : QDialog(parent), m_model(model)
{
    setWindowTitle("灵感回收站");
    resize(760, 400);
    setupUI();
    refresh();
}

void InspirationRecycleBinDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("共 0 条记录", this);
    topLayout->addWidget(m_statusLabel);
    topLayout->addStretch();

    QPushButton *emptyBtn = new QPushButton("清空回收站", this);
    emptyBtn->setIcon(QIcon(":/icons/delete_icon.png"));
    connect(emptyBtn, &QPushButton::clicked, this, &InspirationRecycleBinDialog::onEmpty);
    topLayout->addWidget(emptyBtn);
    layout->addLayout(topLayout);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);

    m_table->setHorizontalHeaderLabels({"记录时间", "内容预览", "标签", "删除时间"});

    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);

    m_table->setColumnWidth(0, 140);
    m_table->setColumnWidth(1, 340);
    m_table->setColumnWidth(2, 120);

    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *restoreBtn = new QPushButton("恢复选中", this);
    QPushButton *deleteBtn = new QPushButton("永久删除", this);
    QPushButton *closeBtn = new QPushButton("关闭", this);

    connect(restoreBtn, &QPushButton::clicked, this, &InspirationRecycleBinDialog::onRestore);
    connect(deleteBtn, &QPushButton::clicked, this, &InspirationRecycleBinDialog::onDelete);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addStretch();
    btnLayout->addWidget(restoreBtn);
    btnLayout->addWidget(deleteBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
}

void InspirationRecycleBinDialog::refresh()
{
    m_table->setRowCount(0);
    QList<QVariantMap> list = m_model->getDeletedInspirations();
    m_statusLabel->setText(QString("共 %1 条已删除记录").arg(list.size()));

    for(const QVariantMap &data : list) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        QDateTime createTime = data["created_at"].toDateTime();
        QTableWidgetItem *timeItem = new QTableWidgetItem(createTime.toString("yyyy-MM-dd HH:mm"));
        timeItem->setData(Qt::UserRole, data["id"]);
        timeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, timeItem);

        QTableWidgetItem *contentItem = new QTableWidgetItem(data["content"].toString());
        contentItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        contentItem->setToolTip(data["content"].toString());
        m_table->setItem(row, 1, contentItem);

        QTableWidgetItem *tagItem = new QTableWidgetItem(data["tags"].toString());
        tagItem->setTextAlignment(Qt::AlignCenter);
        tagItem->setToolTip(data["tags"].toString());
        m_table->setItem(row, 2, tagItem);

        QDateTime deleteTime = data["updated_at"].toDateTime();
        QTableWidgetItem *delTimeItem = new QTableWidgetItem(deleteTime.toString("yyyy-MM-dd HH:mm"));
        delTimeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 3, delTimeItem);
    }
}

void InspirationRecycleBinDialog::onRestore()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();

    if (m_model->restoreInspiration(id)) {
        refresh();
    }
}

void InspirationRecycleBinDialog::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();

    if (QMessageBox::question(this, "确认", "确定要永久删除吗？此操作不可恢复。") == QMessageBox::Yes) {
        if (m_model->permanentDeleteInspiration(id)) {
            refresh();
        }
    }
}

void InspirationRecycleBinDialog::onEmpty()
{
    if (QMessageBox::question(this, "确认", "确定要清空回收站吗？") == QMessageBox::Yes) {
        m_model->emptyRecycleBin();
        refresh();
    }
}
