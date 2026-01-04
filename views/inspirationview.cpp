#include "inspirationview.h"
#include "models/inspirationmodel.h"
#include "dialogs/inspirationdialog.h"
#include "views/calenderview.h"
#include "models/taskmodel.h"
#include "dialogs/inspirationrecyclebindialog.h"
#include "dialogs/inspirationtagsearchdialog.h"
#include <QPainter>
#include <QDateTime>
#include <QListWidget>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>

InspirationGridDelegate::InspirationGridDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void InspirationGridDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QVariantMap data = index.data(Qt::UserRole).toMap();
    QString content = data["content"].toString();
    QDateTime createTime = data["created_at"].toDateTime();
    QString timeStr = createTime.isValid() ? createTime.toString("MM-dd HH:mm") : "";

    QRect rect = option.rect.adjusted(6, 6, -6, -6);

    QColor bgColor;
    QColor borderColor;

    if (option.state & QStyle::State_Selected) {
        bgColor = QColor(100, 125, 160, 160);
        borderColor = QColor(160, 170, 230, 220);
    } else if (option.state & QStyle::State_MouseOver) {
        bgColor = QColor(150, 185, 220, 110);
        borderColor = QColor(255, 255, 255, 160);
    } else {
        bgColor = QColor(135, 160, 190, 90);
        borderColor = QColor(255, 255, 255, 50);
    }

    painter->setBrush(bgColor);
    painter->setPen(QPen(borderColor, 1));
    painter->drawRoundedRect(rect, 10, 10);

    painter->setPen(QColor(245, 245, 255));
    QRect textRect = rect.adjusted(12, 12, -12, -25);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, content);

    painter->setPen(QColor(255, 255, 255, 140));
    QFont timeFont = painter->font();
    timeFont.setPointSizeF(9);
    painter->setFont(timeFont);
    painter->drawText(rect.adjusted(0, 0, -10, -8), Qt::AlignRight | Qt::AlignBottom, timeStr);

    painter->restore();
}

QSize InspirationGridDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(165, 120);
}

InspirationView::InspirationView(QWidget *parent)
    : QWidget(parent), m_model(nullptr)
{
    setupUI();
}

void InspirationView::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout *toolLayout = new QHBoxLayout();

    QPushButton *addBtn = new QPushButton("记录", this);
    addBtn->setObjectName("addInspirationBtn");
    addBtn->setIcon(QIcon(":/icons/add_icon.png"));

    QPushButton *editBtn = new QPushButton("编辑", this);
    editBtn->setObjectName("editInspirationBtn");
    editBtn->setIcon(QIcon(":/icons/edit_icon.png"));

    QPushButton *deleteBtn = new QPushButton("删除", this);
    deleteBtn->setObjectName("deleteInspirationBtn");
    deleteBtn->setIcon(QIcon(":/icons/delete_icon.png"));

    toolLayout->addWidget(addBtn);
    toolLayout->addWidget(editBtn);
    toolLayout->addWidget(deleteBtn);

    toolLayout->addSpacing(10);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索灵感内容...");
    m_searchEdit->setFixedWidth(460);
    toolLayout->addWidget(m_searchEdit);
    toolLayout->addStretch();

    QPushButton *tagSearchBtn = new QPushButton("标签检索", this);
    tagSearchBtn->setObjectName("inspirationTagSearchBtn");
    tagSearchBtn->setIcon(QIcon(":/icons/edit_icon.png"));

    toolLayout->addWidget(tagSearchBtn);

    QPushButton *recycleBinBtn = new QPushButton("回收站", this);
    recycleBinBtn->setObjectName("inspirationRecycleBinBtn");
    recycleBinBtn->setIcon(QIcon(":/icons/recycle_icon.png"));

    QPushButton *refreshBtn = new QPushButton("刷新", this);
    refreshBtn->setObjectName("inspirationRefreshBtn");
    refreshBtn->setIcon(QIcon(":/icons/refresh_icon.png"));

    toolLayout->addWidget(recycleBinBtn);
    toolLayout->addWidget(refreshBtn);

    layout->addLayout(toolLayout);

    m_viewStack = new QStackedWidget(this);

    m_tableView = new QTableView(this);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setShowGrid(false);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_viewStack->addWidget(m_tableView);

    m_gridView = new QListWidget(this);
    m_gridView->setViewMode(QListWidget::IconMode);
    m_gridView->setResizeMode(QListWidget::Adjust);
    m_gridView->setSpacing(8);
    m_gridView->setMovement(QListWidget::Static);
    m_gridView->setSelectionMode(QListWidget::SingleSelection);
    m_gridView->setStyleSheet("QListWidget { background-color: transparent; border: none; }");
    m_gridView->setItemDelegate(new InspirationGridDelegate(this));

    QFont font = m_gridView->font();
    font.setPointSize(10);
    m_gridView->setFont(font);

    m_viewStack->addWidget(m_gridView);

    m_calendarView = new CalendarView(this);
    m_viewStack->addWidget(m_calendarView);

    connect(m_calendarView, &CalendarView::showInspirations,
            this, &InspirationView::showInspirationsRequested);
    connect(m_calendarView, &CalendarView::showTasks,
            this, &InspirationView::showTasksRequested);

    layout->addWidget(m_viewStack);

    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_leftBottomContainer = new QWidget(this);
    m_leftBottomContainer->setFixedWidth(195);
    QHBoxLayout *leftLayout = new QHBoxLayout(m_leftBottomContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);

    m_dateFilterCheck = new QCheckBox("筛选:", this);
    m_dateFilterCheck->setChecked(true);
    m_dateFilterCheck->setStyleSheet("color: #cccccc;");

    m_yearSpin = new QSpinBox(this);
    m_yearSpin->setRange(2000, 2099);
    m_yearSpin->setValue(QDate::currentDate().year());
    m_yearSpin->setSuffix("年");
    m_yearSpin->setFixedWidth(70);
    m_yearSpin->setStyleSheet(
        "QSpinBox { background-color: #2d2d2d; color: #ffffff; border: 1px solid #3d3d3d; border-radius: 4px; padding: 2px; }"
        "QSpinBox::up-button, QSpinBox::down-button { width: 16px; }"
        );

    m_monthSpin = new QSpinBox(this);
    m_monthSpin->setRange(1, 12);
    m_monthSpin->setValue(QDate::currentDate().month());
    m_monthSpin->setSuffix("月");
    m_monthSpin->setFixedWidth(55);
    m_monthSpin->setStyleSheet(m_yearSpin->styleSheet());

    connect(m_dateFilterCheck, &QCheckBox::toggled, this, [this](bool checked){
        m_yearSpin->setEnabled(checked);
        m_monthSpin->setEnabled(checked);
        applyFilters();
    });

    connect(m_yearSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int){ applyFilters(); });
    connect(m_monthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int){ applyFilters(); });

    leftLayout->addWidget(m_dateFilterCheck);
    leftLayout->addWidget(m_yearSpin);
    leftLayout->addWidget(m_monthSpin);

    QButtonGroup *viewGroup = new QButtonGroup(this);

    QPushButton *listViewBtn = new QPushButton("列表视图", this);
    listViewBtn->setCheckable(true);
    listViewBtn->setChecked(true);
    listViewBtn->setObjectName("listViewBtn");

    QPushButton *gridViewBtn = new QPushButton("便利贴墙", this);
    gridViewBtn->setCheckable(true);
    gridViewBtn->setObjectName("kanbanViewBtn");

    QPushButton *calendarViewBtn = new QPushButton("日历视图", this);
    calendarViewBtn->setCheckable(true);
    calendarViewBtn->setObjectName("calendarViewBtn");

    viewGroup->addButton(listViewBtn, 0);
    viewGroup->addButton(gridViewBtn, 1);
    viewGroup->addButton(calendarViewBtn, 2);

    QHBoxLayout *centerBtnLayout = new QHBoxLayout();
    centerBtnLayout->addWidget(listViewBtn);
    centerBtnLayout->addWidget(gridViewBtn);
    centerBtnLayout->addWidget(calendarViewBtn);

    QWidget *rightDummy = new QWidget(this);
    rightDummy->setFixedWidth(195);

    bottomLayout->addWidget(m_leftBottomContainer);
    bottomLayout->addStretch();
    bottomLayout->addLayout(centerBtnLayout);
    bottomLayout->addStretch();
    bottomLayout->addWidget(rightDummy);

    layout->addLayout(bottomLayout);

    connect(viewGroup, &QButtonGroup::idClicked, this, [this, rightDummy](int id){
        m_viewStack->setCurrentIndex(id);

        bool showDateFilter = (id != 2);
        m_leftBottomContainer->setVisible(showDateFilter);
        rightDummy->setVisible(showDateFilter);
    });

    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &InspirationView::onSearchTextChanged);

    connect(m_tableView, &QTableView::doubleClicked,
            this, &InspirationView::onDoubleClicked);

    connect(m_gridView, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){
        int id = item->data(Qt::UserRole).toMap()["id"].toInt();
        for(int i = 0; i < m_model->rowCount(); ++i) {
            QModelIndex idx = m_model->index(i, 0);
            if (m_model->data(idx, Qt::UserRole).toMap()["id"].toInt() == id) {
                onDoubleClicked(idx);
                break;
            }
        }
    });

    connect(addBtn, &QPushButton::clicked, this, &InspirationView::onAddClicked);
    connect(editBtn, &QPushButton::clicked, this, &InspirationView::onEditClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &InspirationView::onDeleteClicked);
    connect(refreshBtn, &QPushButton::clicked, this, &InspirationView::refresh);
    connect(tagSearchBtn, &QPushButton::clicked, this, &InspirationView::onTagSearchClicked);

    connect(recycleBinBtn, &QPushButton::clicked, this, [this](){
        if (!m_model) return;
        InspirationRecycleBinDialog dlg(m_model, this);
        dlg.exec();
        refresh();
    });
}

void InspirationView::setModel(InspirationModel *model)
{
    m_model = model;
    m_tableView->setModel(model);

    m_tableView->horizontalHeader()->setMinimumSectionSize(100);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setColumnWidth(0, 160);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tableView->setColumnWidth(1, 650);
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);

    m_calendarView->setInspirationModel(model);

    refreshGridView();

    connect(model, &QAbstractTableModel::modelReset, this, &InspirationView::refreshGridView);
    connect(model, &QAbstractTableModel::rowsInserted, this, &InspirationView::refreshGridView);
    connect(model, &QAbstractTableModel::rowsRemoved, this, &InspirationView::refreshGridView);
    connect(model, &QAbstractTableModel::dataChanged, this, &InspirationView::refreshGridView);
}

void InspirationView::refresh()
{
    if (m_model) {
        m_model->refresh();

        m_filterTags.clear();
        m_filterMatchAll = false;
        m_searchEdit->clear();

        QDate current = QDate::currentDate();
        m_yearSpin->blockSignals(true);
        m_monthSpin->blockSignals(true);
        m_yearSpin->setValue(current.year());
        m_monthSpin->setValue(current.month());
        m_dateFilterCheck->setChecked(true);
        m_yearSpin->blockSignals(false);
        m_monthSpin->blockSignals(false);

        applyFilters();
    }
}

void InspirationView::onTagSearchClicked()
{
    if (!m_model) return;

    InspirationTagSearchDialog dlg(m_model, m_filterTags, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_filterTags = dlg.getSelectedTags();
        m_filterMatchAll = dlg.isMatchAll();
        applyFilters();
    }
}

void InspirationView::applyFilters()
{
    if (!m_model) return;

    if (m_calendarView) {
        m_calendarView->setInspirationFilter(m_filterTags, m_filterMatchAll);
    }

    QString search = m_searchEdit->text();

    bool filterDate = m_dateFilterCheck->isChecked();
    int targetYear = m_yearSpin->value();
    int targetMonth = m_monthSpin->value();

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idx = m_model->index(i, 0);
        QVariantMap data = m_model->data(idx, Qt::UserRole).toMap();

        QString content = data["content"].toString();
        QString tagsStr = data["tags"].toString();
        QDateTime createTime = data["created_at"].toDateTime();
        QStringList itemTags = tagsStr.split(",", Qt::SkipEmptyParts);

        bool dateMatch = true;
        if (filterDate) {
            if (createTime.date().year() != targetYear ||
                createTime.date().month() != targetMonth) {
                dateMatch = false;
            }
        }

        bool tagMatch = true;
        if (!m_filterTags.isEmpty()) {
            if (m_filterMatchAll) {
                for (const QString &filterTag : m_filterTags) {
                    bool found = false;
                    for (const QString &itemTag : itemTags) {
                        if (itemTag.trimmed() == filterTag) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        tagMatch = false;
                        break;
                    }
                }
            } else {
                tagMatch = false;
                for (const QString &filterTag : m_filterTags) {
                    for (const QString &itemTag : itemTags) {
                        if (itemTag.trimmed() == filterTag) {
                            tagMatch = true;
                            break;
                        }
                    }
                    if (tagMatch) break;
                }
            }
        }

        bool searchMatch = search.isEmpty() ||
                           content.contains(search, Qt::CaseInsensitive) ||
                           tagsStr.contains(search, Qt::CaseInsensitive);

        bool shouldShow = dateMatch && tagMatch && searchMatch;

        m_tableView->setRowHidden(i, !shouldShow);

        if (m_gridView && i < m_gridView->count()) {
            m_gridView->item(i)->setHidden(!shouldShow);
        }
    }
}

void InspirationView::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    applyFilters();
}

void InspirationView::onDoubleClicked(const QModelIndex &index)
{
    if (!m_model) return;

    QVariantMap data = m_model->data(index, Qt::UserRole).toMap();
    InspirationDialog dialog(data, this);

    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap newData = dialog.getData();

        if (newData.contains("id")) {
            m_model->updateInspiration(newData["id"].toInt(), newData["content"].toString(), newData["tags"].toString());
        } else {
            m_model->addInspiration(newData["content"].toString(), newData["tags"].toString());
        }

        applyFilters();
    }
}
void InspirationView::onAddClicked()
{
    InspirationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap data = dialog.getData();
        m_model->addInspiration(data["content"].toString(), data["tags"].toString());
        m_tableView->scrollToTop();
    }
}

void InspirationView::onDeleteClicked()
{
    QModelIndex index = m_tableView->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "提示", "请先选择一条记录");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除这条灵感吗？") == QMessageBox::Yes) {
        int id = m_model->data(index, Qt::UserRole).toMap()["id"].toInt();
        m_model->deleteInspiration(id);
    }
}

void InspirationView::onEditClicked()
{
    QModelIndex index = m_tableView->currentIndex();
    if (index.isValid()) {
        onDoubleClicked(index);
    } else {
        QMessageBox::warning(this, "提示", "请先选择一条灵感记录");
    }
}

void InspirationView::refreshGridView()
{
    if (!m_gridView || !m_model) return;

    m_gridView->clear();
    QList<QVariantMap> inspirations = m_model->getAllInspirations();

    for (const QVariantMap &data : inspirations) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, data);
        m_gridView->addItem(item);
    }
}

void InspirationView::setTaskModel(TaskModel *model)
{
    if (m_calendarView) {
        m_calendarView->setTaskModel(model);
    }
}


