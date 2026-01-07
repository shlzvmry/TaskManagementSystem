#include "kanbanview.h"
#include "models/taskmodel.h"
#include "models/taskfiltermodel.h"
#include "database/database.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>
#include <QDrag>
#include <QComboBox>

KanbanDelegate::KanbanDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize KanbanDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(200, 72);
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

    QString bgMode = Database::instance().getSetting("bg_mode", "dark");
    bool isLight = (bgMode == "light");

    QColor cardBgColor, textColor, subTextColor, tagBgColor, borderColor;

    if (isLight) {
        cardBgColor = QColor("#ffffff");
        textColor = QColor("#303133");
        subTextColor = QColor("#909399");
        tagBgColor = QColor("#f5f7fa");
        borderColor = QColor("#dcdfe6");
    } else {
        cardBgColor = QColor("#262626");
        textColor = QColor("#ffffff");
        subTextColor = QColor("#cccccc");
        tagBgColor = QColor("#333333");
        borderColor = QColor("#3d3d3d");
    }

    QRect rect = option.rect.adjusted(4, 3, -4, -3);

    if (option.state & QStyle::State_Selected) {
        QString themeColorStr = Database::instance().getSetting("theme_color", "#657896");
        QColor themeColor(themeColorStr);
        themeColor.setAlpha(isLight ? 40 : 60);
        painter->setBrush(themeColor);
        painter->setPen(QColor(themeColorStr));
    } else {
        painter->setBrush(cardBgColor);
        painter->setPen(borderColor);
    }

    painter->drawRoundedRect(rect, 4, 4);

    QRect colorStrip = rect;
    colorStrip.setWidth(4);
    painter->setBrush(priorityColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(colorStrip.adjusted(0,0,0,0), 4, 4, Qt::AbsoluteSize);
    painter->drawRect(colorStrip.adjusted(2,0,0,0));

    int leftPadding = 14;
    int rightPadding = 8;

    QFont titleFont = painter->font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    painter->setFont(titleFont);
    painter->setPen(textColor);

    QRect titleRect = rect.adjusted(leftPadding, 8, -rightPadding, -28);
    QString elidedTitle = painter->fontMetrics().elidedText(title, Qt::ElideRight, titleRect.width());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, elidedTitle);

    QFont tagFont = painter->font();
    tagFont.setPointSize(8);
    painter->setFont(tagFont);
    QFontMetrics fm(tagFont);

    int catWidth = fm.horizontalAdvance(category) + 12;
    int catHeight = 16;
    QRect catRect(rect.left() + leftPadding, rect.bottom() - 8 - catHeight, catWidth, catHeight);

    painter->setBrush(tagBgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(catRect, 3, 3);
    painter->setPen(subTextColor);
    painter->drawText(catRect, Qt::AlignCenter, category);

    QString timeStr;
    QString prefix;

    if (status == 2) {
        if (completedAt.isValid()) {
            timeStr = completedAt.toString("MM-dd");
            prefix = "√ ";
        }
    } else {
        if (deadline.isValid()) {
            timeStr = deadline.toString("MM-dd");
            if (index.data(TaskModel::IsOverdueRole).toBool()) {
                painter->setPen(QColor("#FF6B6B"));
            } else {
                painter->setPen(subTextColor);
            }
        }
    }

    if (!timeStr.isEmpty()) {
        QString fullText = prefix + timeStr;
        if (painter->pen().color() != QColor("#FF6B6B")) {
            painter->setPen(subTextColor);
        }
        painter->drawText(rect.adjusted(0, 0, -rightPadding, -8), Qt::AlignRight | Qt::AlignBottom, fullText);
    }

    painter->restore();
}

KanbanColumn::KanbanColumn(int value, QWidget *parent)
    : QListView(parent), m_value(value)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformItemSizes(true);

    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setResizeMode(QListView::Adjust);

    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSpacing(2);

    setItemDelegate(new KanbanDelegate(this));

    connect(this, &QListView::doubleClicked, this, [this](const QModelIndex &index){
        int taskId = index.data(TaskModel::IdRole).toInt();
        if (taskId > 0) {
            emit taskDoubleClicked(taskId);
        }
    });
}


void KanbanColumn::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-task-id")) {
        QByteArray encodedData = event->mimeData()->data("application/x-task-id");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        int taskId = 0;
        stream >> taskId;

        if (taskId > 0) {
            emit taskDropped(taskId, m_value);
            event->accept();
        }
    } else {
        event->ignore();
    }
}

void KanbanColumn::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) return;

    QMimeData *mimeData = model()->mimeData(indexes);
    if (!mimeData) return;

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    QPixmap pixmap(viewport()->visibleRegion().boundingRect().size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    drag->exec(Qt::MoveAction);
}

void KanbanColumn::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-task-id")) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->ignore();
    }
}

void KanbanColumn::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-task-id")) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->ignore();
    }
}

KanbanView::KanbanView(QWidget *parent)
    : QWidget(parent)
    , m_model(nullptr)
    , m_groupMode(GroupByStatus)
{
    setupUI();
}

void KanbanView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QWidget *columnContainer = new QWidget(this);
    m_columnLayout = new QHBoxLayout(columnContainer);
    m_columnLayout->setSpacing(10);
    m_columnLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(columnContainer);

    refreshColumns();
}

void KanbanView::setGroupMode(GroupMode mode)
{
    if (m_groupMode != mode) {
        m_groupMode = mode;
        refreshColumns();
    }
}

void KanbanView::refreshColumns()
{
    qDeleteAll(m_columns);
    m_columns.clear();
    qDeleteAll(m_filters);
    m_filters.clear();

    QLayoutItem *item;
    while ((item = m_columnLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    if (m_groupMode == GroupByStatus) {
        createColumn("待办", 0, "#3498db");
        createColumn("进行中", 1, "#e67e22");
        createColumn("已完成", 2, "#2ecc71");
        createColumn("已延期", 3, "#e74c3c");
    } else {
        createColumn("紧急", 0, "#FF4444");
        createColumn("重要", 1, "#FF9900");
        createColumn("普通", 2, "#4CAF50");
        createColumn("不急", 3, "#9E9E9E");
    }

    if (m_model) {
        setModel(m_model);
    }
}

KanbanColumn* KanbanView::createColumn(const QString &title, int value, const QString &color)
{
    QWidget *columnWidget = new QWidget(this);
    columnWidget->setObjectName("kanbanColumn");
    columnWidget->setStyleSheet(QString("#kanbanColumn { border-top: 3px solid %1; }").arg(color));

    QVBoxLayout *layout = new QVBoxLayout(columnWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *header = new QLabel(title, columnWidget);
    header->setAlignment(Qt::AlignCenter);
    header->setStyleSheet("font-weight: bold; font-size: 14px; padding: 8px;");
    layout->addWidget(header);

    KanbanColumn *list = new KanbanColumn(value, columnWidget);
    layout->addWidget(list);

    m_columnLayout->addWidget(columnWidget);
    m_columns.append(list);

    return list;
}

void KanbanView::setModel(TaskModel *model)
{
    m_model = model;
    if (!m_model) return;

    qDeleteAll(m_filters);
    m_filters.clear();

    for (KanbanColumn *col : m_columns) {
        TaskFilterModel *filter = new TaskFilterModel(this);
        filter->setSourceModel(model);

        if (m_groupMode == GroupByStatus) {
            filter->setFilterMode(TaskFilterModel::FilterStatus);
            filter->setFilterStatus(col->getValue());
            filter->setFilterPriority(-1);
        } else {
            filter->setFilterMode(TaskFilterModel::FilterAll);
            filter->setFilterPriority(col->getValue());
            filter->setFilterStatus(-1);
        }

        col->setModel(filter);
        m_filters.append(filter);

        connect(col, &KanbanColumn::taskDropped, this, [this](int taskId, int newValue){
            QVariantMap data;
            if (m_groupMode == GroupByStatus) {
                data["status"] = newValue;
            } else {
                data["priority"] = newValue;
            }
            m_model->updateTask(taskId, data);
        });

        connect(col, &KanbanColumn::taskDoubleClicked, this, &KanbanView::editTaskRequested);
    }
}
