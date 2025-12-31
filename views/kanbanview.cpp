#include "kanbanview.h"
#include "models/taskmodel.h"
#include "models/taskfiltermodel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>


KanbanDelegate::KanbanDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize KanbanDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(260, 110);
}

void KanbanDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QString title = index.data(TaskModel::TitleRole).toString();
    QString category = index.data(TaskModel::CategoryNameRole).toString();
    QColor priorityColor = index.data(TaskModel::PriorityColorRole).value<QColor>();

    int status = index.data(TaskModel::StatusRole).toInt();
    QDateTime deadline = index.data(TaskModel::DeadlineRole).toDateTime();
    QDateTime completedAt = index.data(TaskModel::CompletedAtRole).toDateTime();

    // 绘制背景
    QRect rect = option.rect.adjusted(4, 4, -4, -4);

    QColor cardBgColor = QColor("#3d3d3d");
    QColor textColor = QColor("#e0e0e0");
    QColor subTextColor = QColor("#aaaaaa");
    QColor tagBgColor = QColor("#4d4d4d");

    // 选中状态
    if (option.state & QStyle::State_Selected) {
        painter->setBrush(QColor("#4a5a6d"));
        painter->setPen(QColor("#657896"));
    } else {
        painter->setBrush(cardBgColor);
        painter->setPen(Qt::NoPen);
    }

    painter->drawRoundedRect(rect, 6, 6);

    QRect colorStrip = rect;
    colorStrip.setWidth(5);
    painter->setBrush(priorityColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(colorStrip.adjusted(0,0,0,0), 6, 6, Qt::AbsoluteSize);
    painter->drawRect(colorStrip.adjusted(3,0,0,0));
    // 绘制内容
    int leftPadding = 15;
    int topPadding = 12;

    //标题
    QFont titleFont = painter->font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    painter->setFont(titleFont);
    painter->setPen(textColor);

    QRect titleRect = rect.adjusted(leftPadding, topPadding, -10, -40);
    QString elidedTitle = painter->fontMetrics().elidedText(title, Qt::ElideRight, titleRect.width());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, elidedTitle);

    // 分类标签
    QFont tagFont = painter->font();
    tagFont.setPointSize(9);
    painter->setFont(tagFont);

    QFontMetrics fm(tagFont);
    int catWidth = fm.horizontalAdvance(category) + 16;
    QRect catRect(rect.left() + leftPadding, rect.bottom() - 28, catWidth, 20);

    painter->setBrush(tagBgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(catRect, 4, 4);

    painter->setPen(subTextColor);
    painter->drawText(catRect, Qt::AlignCenter, category);

    // 3. 时间显示
    QString timeStr;
    QString prefix;

    if (status == 2) { // 已完成
        if (completedAt.isValid()) {
            timeStr = completedAt.toString("MM-dd HH:mm");
            prefix = "完成: ";
        }
    } else { // 其他状态
        if (deadline.isValid()) {
            timeStr = deadline.toString("MM-dd HH:mm");
            prefix = "截止: ";
        }
    }

    if (!timeStr.isEmpty()) {
        painter->setPen(subTextColor);
        QString fullText = prefix + timeStr;
        painter->drawText(rect.adjusted(0, 0, -15, -8), Qt::AlignRight | Qt::AlignBottom, fullText);
    }

    painter->restore();
}

KanbanColumn::KanbanColumn(int status, QWidget *parent)
    : QListView(parent), m_status(status)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeMode(QListView::Adjust);
    // -------------------------

    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSpacing(5);
    setStyleSheet("QListView { background-color: transparent; border: none; }");

    setItemDelegate(new KanbanDelegate(this));
}

void KanbanColumn::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->accept();
    } else {
        event->ignore();
    }
}

void KanbanColumn::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->ignore();
    }
}

void KanbanColumn::dropEvent(QDropEvent *event)
{
    // 获取拖拽的数据
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        int row, col;
        QMap<int, QVariant> roleDataMap;
        while (!stream.atEnd()) {
            stream >> row >> col >> roleDataMap;
            if (roleDataMap.contains(TaskModel::IdRole)) {
                int taskId = roleDataMap[TaskModel::IdRole].toInt();
                emit taskDropped(taskId, m_status);
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

KanbanView::KanbanView(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void KanbanView::setupUI()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(10, 10, 10, 10);

    // 创建四列：待办、进行中、已完成、已延期
    createColumn("待办", 0, "#3498db");
    createColumn("进行中", 1, "#e67e22");
    createColumn("已完成", 2, "#2ecc71");
    createColumn("已延期", 3, "#e74c3c");
}

KanbanColumn* KanbanView::createColumn(const QString &title, int status, const QString &color)
{
    QWidget *columnWidget = new QWidget(this);
    columnWidget->setObjectName("kanbanColumn");
    columnWidget->setStyleSheet(QString("#kanbanColumn { border-top: 3px solid %1; }").arg(color));

    QVBoxLayout *layout = new QVBoxLayout(columnWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    // 标题栏
    QLabel *header = new QLabel(title, columnWidget);
    header->setAlignment(Qt::AlignCenter);
    header->setStyleSheet("font-weight: bold; font-size: 14px; padding: 10px; color: #555;");
    layout->addWidget(header);

    // 列表
    KanbanColumn *list = new KanbanColumn(status, columnWidget);
    layout->addWidget(list);

    m_layout->addWidget(columnWidget);
    m_columns.append(list);

    return list;
}

void KanbanView::setModel(TaskModel *model)
{
    m_model = model;

    for (KanbanColumn *col : m_columns) {
        TaskFilterModel *filter = new TaskFilterModel(this);
        filter->setSourceModel(model);
        filter->setFilterMode(TaskFilterModel::FilterStatus);
        filter->setFilterStatus(col->getStatus());

        col->setModel(filter);
        m_filters.append(filter);

        connect(col, &KanbanColumn::taskDropped, this, [this](int taskId, int newStatus){
            QVariantMap data;
            data["status"] = newStatus;
            m_model->updateTask(taskId, data);
        });
    }
}
