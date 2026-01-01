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
    // 连接双击信号
    connect(this, &QTableView::doubleClicked, this, &TaskTableView::onDoubleClicked);
}

void TaskTableView::setupUI()
{
    // 基础属性设置
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);
    setSortingEnabled(true);
    setEditTriggers(QAbstractItemView::AllEditTriggers); // 允许点击触发代理编辑(如优先级下拉框)
    setFrameShape(QFrame::NoFrame);

    // 垂直表头隐藏
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(35);

    // 水平表头设置
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setHighlightSections(false);
}

void TaskTableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);

    // 设置自定义代理 (优先级和状态列)
    ComboBoxDelegate *delegate = new ComboBoxDelegate(this);
    setItemDelegateForColumn(3, delegate); // 优先级
    setItemDelegateForColumn(4, delegate); // 状态

    // 设置列宽
    setColumnWidth(0, 50);   // ID
    setColumnWidth(1, 220);  // 标题
    setColumnWidth(2, 100);  // 分类
    setColumnWidth(3, 80);   // 优先级
    setColumnWidth(4, 80);   // 状态
    setColumnWidth(5, 140);  // 截止时间
    setColumnWidth(6, 140);  // 提醒时间
    setColumnWidth(7, 140);  // 创建/完成时间

    // 默认按截止时间排序
    sortByColumn(5, Qt::AscendingOrder);
}

void TaskTableView::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    // 如果点击的是可编辑列(优先级/状态)，不触发弹窗编辑，交由代理处理
    if (index.column() == 3 || index.column() == 4) {
        return;
    }

    // 处理代理模型索引映射
    QModelIndex sourceIndex = index;
    if (QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel*>(model())) {
        sourceIndex = proxy->mapToSource(index);
    }

    // 获取任务ID并发送信号
    int taskId = sourceIndex.data(TaskModel::IdRole).toInt();
    if (taskId > 0) {
        emit editTaskRequested(taskId);
    }
}
