#include "tasktableview.h"
#include "widgets/comboboxdelegate.h"
#include "models/taskmodel.h"
#include <QHeaderView>
#include <QAbstractProxyModel>
#include <QDebug>

TaskTableView::TaskTableView(QWidget *parent)
    : QTableView(parent)
{
    setupUI();
    connect(this, &QTableView::doubleClicked, this, &TaskTableView::onDoubleClicked);
}

void TaskTableView::setupUI()
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);
    setSortingEnabled(true);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    setFrameShape(QFrame::NoFrame);

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(35);

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setHighlightSections(false);
}

void TaskTableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);

    ComboBoxDelegate *delegate = new ComboBoxDelegate(this);
    setItemDelegateForColumn(3, delegate);
    setItemDelegateForColumn(4, delegate);

    setColumnWidth(0, 50);
    setColumnWidth(1, 240);
    setColumnWidth(2, 100);
    setColumnWidth(3, 90);
    setColumnWidth(4, 90);
    setColumnWidth(5, 160);
    setColumnWidth(6, 160);
    setColumnWidth(7, 160);

    sortByColumn(5, Qt::AscendingOrder);
}

void TaskTableView::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    if (index.column() == 3 || index.column() == 4) {
        return;
    }

    QModelIndex sourceIndex = index;
    if (QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel*>(model())) {
        sourceIndex = proxy->mapToSource(index);
    }

    int taskId = sourceIndex.data(TaskModel::IdRole).toInt();
    if (taskId > 0) {
        emit editTaskRequested(taskId);
    }
}
