#include "inspirationview.h"
#include "models/inspirationmodel.h"
#include "dialogs/inspirationdialog.h"
#include "views/calenderview.h"
#include "models/taskmodel.h"
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

    // 获取数据
    QVariantMap data = index.data(Qt::UserRole).toMap();
    QString content = data["content"].toString();
    QDateTime createTime = data["created_at"].toDateTime();
    QString timeStr = createTime.isValid() ? createTime.toString("MM-dd HH:mm") : "";

    QRect rect = option.rect.adjusted(4, 4, -4, -4);

    // 绘制背景 (淡黄色便利贴风格)
    if (option.state & QStyle::State_Selected) {
        painter->setBrush(QColor("#fff59d")); // 选中稍微深一点
        painter->setPen(QColor("#fbc02d"));
    } else {
        painter->setBrush(QColor("#fff9c4")); // 默认淡黄
        painter->setPen(Qt::NoPen);
    }
    painter->drawRoundedRect(rect, 8, 8);

    // 绘制内容
    painter->setPen(QColor("#5d4037")); // 深褐色文字
    QRect textRect = rect.adjusted(8, 8, -8, -25);

    // 简单的文本自动换行处理
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    painter->drawText(textRect, content, textOption);

    // 绘制时间 (右下角)
    painter->setPen(QColor("#a1887f"));
    painter->setFont(QFont(painter->font().family(), 8));
    painter->drawText(rect.adjusted(0, 0, -8, -5), Qt::AlignRight | Qt::AlignBottom, timeStr);

    painter->restore();
}

QSize InspirationGridDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(160, 160); // 固定卡片大小
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

    // --- 1. 顶部工具栏 ---
    QHBoxLayout *toolLayout = new QHBoxLayout();

    // 左侧：增删改
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

    // 中间：过滤与搜索
    toolLayout->addSpacing(20);
    toolLayout->addWidget(new QLabel("过滤:", this));

    m_tagCombo = new QComboBox(this);
    m_tagCombo->addItem("所有标签", "");
    m_tagCombo->setFixedWidth(120);
    m_tagCombo->setObjectName("filterTagCombo");

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索灵感内容...");
    m_searchEdit->setFixedWidth(190);

    toolLayout->addWidget(m_tagCombo);
    toolLayout->addWidget(m_searchEdit);

    // 右侧：功能按钮
    toolLayout->addStretch();

    QPushButton *recycleBinBtn = new QPushButton("回收站", this);
    recycleBinBtn->setObjectName("inspirationRecycleBinBtn");
    recycleBinBtn->setIcon(QIcon(":/icons/recycle_icon.png"));

    QPushButton *tagManagerBtn = new QPushButton("标签管理", this);
    tagManagerBtn->setObjectName("inspirationTagManagerBtn");
    tagManagerBtn->setIcon(QIcon(":/icons/edit_icon.png"));

    QPushButton *refreshBtn = new QPushButton("刷新", this);
    refreshBtn->setObjectName("inspirationRefreshBtn");
    refreshBtn->setIcon(QIcon(":/icons/refresh_icon.png"));

    toolLayout->addWidget(recycleBinBtn);
    toolLayout->addWidget(tagManagerBtn);
    toolLayout->addWidget(refreshBtn);

    layout->addLayout(toolLayout);

    // --- 2. 中间视图区域 (StackedWidget) ---
    m_viewStack = new QStackedWidget(this);

    // View 0: 列表视图
    m_tableView = new QTableView(this);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setShowGrid(false);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_viewStack->addWidget(m_tableView);

    // View 1: 网格视图 (便利贴墙)
    m_gridView = new QListWidget(this);
    m_gridView->setViewMode(QListWidget::IconMode);
    m_gridView->setResizeMode(QListWidget::Adjust);
    m_gridView->setSpacing(10);
    m_gridView->setMovement(QListWidget::Static);
    m_gridView->setSelectionMode(QListWidget::SingleSelection);
    m_gridView->setStyleSheet("QListWidget { background-color: transparent; border: none; }");
    m_gridView->setItemDelegate(new InspirationGridDelegate(this));
    m_viewStack->addWidget(m_gridView);

    // View 2: 日历视图
    m_calendarView = new CalendarView(this);
    m_viewStack->addWidget(m_calendarView);

    connect(m_calendarView, &CalendarView::showInspirations, this, &InspirationView::showInspirationsRequested);
    connect(m_calendarView, &CalendarView::showTasks, this, &InspirationView::showTasksRequested);

    layout->addWidget(m_viewStack);

    // --- 3. 底部视图切换栏 ---
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    QButtonGroup *viewGroup = new QButtonGroup(this);
    QPushButton *listViewBtn = new QPushButton("列表视图", this);
    listViewBtn->setCheckable(true);
    listViewBtn->setChecked(true);

    QPushButton *gridViewBtn = new QPushButton("便利贴墙", this);
    gridViewBtn->setCheckable(true);

    QPushButton *calendarViewBtn = new QPushButton("日历视图", this);
    calendarViewBtn->setCheckable(true);

    viewGroup->addButton(listViewBtn, 0);
    viewGroup->addButton(gridViewBtn, 1);
    viewGroup->addButton(calendarViewBtn, 2);

    bottomLayout->addWidget(listViewBtn);
    bottomLayout->addWidget(gridViewBtn);
    bottomLayout->addWidget(calendarViewBtn);
    bottomLayout->addStretch();

    layout->addLayout(bottomLayout);

    // --- 连接信号 ---
    connect(viewGroup, &QButtonGroup::idClicked, m_viewStack, &QStackedWidget::setCurrentIndex);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &InspirationView::onSearchTextChanged);
    connect(m_tagCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InspirationView::onTagFilterChanged);

    // 列表视图交互
    connect(m_tableView, &QTableView::doubleClicked, this, &InspirationView::onDoubleClicked);

    // 网格视图交互 (双击编辑)
    connect(m_gridView, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){
        int id = item->data(Qt::UserRole).toMap()["id"].toInt();
        // 遍历模型找到对应的 Index
        for(int i=0; i<m_model->rowCount(); ++i) {
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

    connect(recycleBinBtn, &QPushButton::clicked, this, [this](){
        QMessageBox::information(this, "提示", "灵感记录为直接删除模式，暂无回收站功能。");
    });
    connect(tagManagerBtn, &QPushButton::clicked, this, [this](){
        QMessageBox::information(this, "提示", "灵感标签为自由文本，请直接在编辑时修改。");
    });
}

void InspirationView::setModel(InspirationModel *model)
{
    m_model = model;
    m_tableView->setModel(model);

    // 设置列表视图列宽
    m_tableView->horizontalHeader()->setMinimumSectionSize(100);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setColumnWidth(0, 160);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tableView->setColumnWidth(1, 630);
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);

    // 设置日历视图的模型
    m_calendarView->setInspirationModel(model);

    // 刷新网格视图
    refreshGridView();

    // 监听模型变化以刷新网格
    connect(model, &QAbstractTableModel::modelReset, this, &InspirationView::refreshGridView);
    connect(model, &QAbstractTableModel::rowsInserted, this, &InspirationView::refreshGridView);
    connect(model, &QAbstractTableModel::rowsRemoved, this, &InspirationView::refreshGridView);
    connect(model, &QAbstractTableModel::dataChanged, this, &InspirationView::refreshGridView);

    updateTagCombo();
}

void InspirationView::refresh()
{
    if (m_model) {
        m_model->refresh();
        updateTagCombo();
    }
}

void InspirationView::updateTagCombo()
{
    if (!m_model) return;

    QString currentTag = m_tagCombo->currentData().toString();

    m_tagCombo->blockSignals(true);
    m_tagCombo->clear();
    m_tagCombo->addItem("所有标签", "");

    QStringList tags = m_model->getAllTags();
    for (const QString &tag : tags) {
        m_tagCombo->addItem(tag, tag);
    }

    int index = m_tagCombo->findData(currentTag);
    if (index != -1) m_tagCombo->setCurrentIndex(index);

    m_tagCombo->blockSignals(false);
}

void InspirationView::onSearchTextChanged(const QString &text)
{
    // 简单实现：遍历隐藏行
    if (!m_model) return;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idxContent = m_model->index(i, 1);
        QModelIndex idxTags = m_model->index(i, 2);
        QString content = m_model->data(idxContent).toString();
        QString tags = m_model->data(idxTags).toString();

        bool match = content.contains(text, Qt::CaseInsensitive) ||
                     tags.contains(text, Qt::CaseInsensitive);

        // 如果搜索框为空，则显示所有
        if (text.isEmpty()) {
            // 重新触发一次标签过滤以恢复状态
            onTagFilterChanged(m_tagCombo->currentIndex());
            return;
        }

        m_tableView->setRowHidden(i, !match);
    }
}

void InspirationView::onTagFilterChanged(int index)
{
    Q_UNUSED(index);
    QString tag = m_tagCombo->currentData().toString();
    QString search = m_searchEdit->text();

    if (!m_model) return;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idxContent = m_model->index(i, 1);
        QModelIndex idxTags = m_model->index(i, 2);
        QString content = m_model->data(idxContent).toString();
        QString tags = m_model->data(idxTags).toString();

        bool tagMatch = tag.isEmpty() || tags.contains(tag, Qt::CaseInsensitive);
        bool searchMatch = search.isEmpty() ||
                           content.contains(search, Qt::CaseInsensitive) ||
                           tags.contains(search, Qt::CaseInsensitive);

        m_tableView->setRowHidden(i, !(tagMatch && searchMatch));
    }
}

void InspirationView::onDoubleClicked(const QModelIndex &index)
{
    if (!m_model) return;

    QVariantMap data = m_model->data(index, Qt::UserRole).toMap();
    InspirationDialog dialog(data, this);

    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap newData = dialog.getData();
        m_model->updateInspiration(newData["id"].toInt(), newData["content"].toString(), newData["tags"].toString());
        updateTagCombo();
    }
}

void InspirationView::onAddClicked()
{
    InspirationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap data = dialog.getData();
        m_model->addInspiration(data["content"].toString(), data["tags"].toString());
        updateTagCombo();
        // 滚动到顶部
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
        updateTagCombo();
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

