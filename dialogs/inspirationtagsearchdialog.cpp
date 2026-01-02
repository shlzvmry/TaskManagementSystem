#include "inspirationtagsearchdialog.h"
#include "models/inspirationmodel.h"
#include "widgets/tagwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QStyle>
#include <QLayout>
#include <QDebug>

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout() {
        QLayoutItem *item;
        while ((item = takeAt(0))) delete item;
    }

    void addItem(QLayoutItem *item) override { itemList.append(item); }
    int horizontalSpacing() const { return m_hSpace >= 0 ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing); }
    int verticalSpacing() const { return m_vSpace >= 0 ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing); }
    int count() const override { return itemList.size(); }
    QLayoutItem *itemAt(int index) const override { return itemList.value(index); }
    QLayoutItem *takeAt(int index) override {
        if (index >= 0 && index < itemList.size()) return itemList.takeAt(index);
        return nullptr;
    }

    Qt::Orientations expandingDirections() const override { return Qt::Orientation(0); }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    void setGeometry(const QRect &rect) override {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override {
        QSize size;
        for (const QLayoutItem *item : itemList)
            size = size.expandedTo(item->minimumSize());
        QMargins margins = contentsMargins();
        size += QSize(margins.left() + margins.right(),
            margins.top() + margins.bottom());
        return size;
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0;

        for (QLayoutItem *item : itemList) {
            QWidget *wid = item->widget();
            int spaceX = horizontalSpacing();
            if (spaceX == -1) spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
            int spaceY = verticalSpacing();
            if (spaceY == -1) spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

            int nextX = x + item->sizeHint().width() + spaceX;
            if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + item->sizeHint().width() + spaceX;
                lineHeight = 0;
            }

            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

            x = nextX;
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        }
        return y + lineHeight - rect.y() + bottom;
    }
    int smartSpacing(QStyle::PixelMetric pm) const {
        QObject *parent = this->parent();
        if (!parent) return -1;
        else if (parent->isWidgetType()) {
            QWidget *pw = static_cast<QWidget *>(parent);
            return pw->style()->pixelMetric(pm, nullptr, pw);
        }
        return -1;
    }

    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
};

InspirationTagSearchDialog::InspirationTagSearchDialog(InspirationModel *model, const QStringList &initialSelection, QWidget *parent)
    : QDialog(parent), m_model(model), m_selectedTags(initialSelection)
{
    setWindowTitle("标签检索");
    resize(500, 400);
    setupUI();
    loadTags();
}

void InspirationTagSearchDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索标签...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &InspirationTagSearchDialog::onSearchTextChanged);
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_containerWidget = new QWidget();
    FlowLayout *flowLayout = new FlowLayout(m_containerWidget, 10, 8, 8);
    m_containerWidget->setLayout(flowLayout);

    m_scrollArea->setWidget(m_containerWidget);
    mainLayout->addWidget(m_scrollArea);

    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_matchAllCheckBox = new QCheckBox("仅显示包含所有选中标签的记录", this);
    bottomLayout->addWidget(m_matchAllCheckBox);
    bottomLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消", this);
    QPushButton *okBtn = new QPushButton("确定", this);
    okBtn->setDefault(true);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &InspirationTagSearchDialog::onConfirm);

    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(okBtn);
    mainLayout->addLayout(bottomLayout);
}

void InspirationTagSearchDialog::loadTags()
{
    m_allTags = m_model->getAllTags();
    m_allTags.sort();
    refreshTagGrid();
}

void InspirationTagSearchDialog::refreshTagGrid()
{
    QLayout *layout = m_containerWidget->layout();
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    m_tagButtons.clear();

    QString filter = m_searchEdit->text().trimmed();

    if (filter.isEmpty()) {
        m_allBtn = new QPushButton("All/清空", m_containerWidget);
        m_allBtn->setObjectName("tagSearchAllBtn");
        m_allBtn->setCheckable(true);
        m_allBtn->setFixedHeight(26);
        m_allBtn->setCursor(Qt::PointingHandCursor);

        m_allBtn->setChecked(m_selectedTags.isEmpty());
        connect(m_allBtn, &QPushButton::clicked, this, &InspirationTagSearchDialog::onAllButtonClicked);
        layout->addWidget(m_allBtn);
    }

    for (const QString &tag : m_allTags) {
        if (!filter.isEmpty() && !tag.contains(filter, Qt::CaseInsensitive)) {
            continue;
        }

        QPushButton *btn = new QPushButton(tag, m_containerWidget);
        btn->setProperty("class", "tagSearchBtn");
        btn->setCheckable(true);
        btn->setFixedHeight(26);
        btn->setCursor(Qt::PointingHandCursor);

        QString color = TagWidget::generateColor(tag);
        QString dynamicStyle = QString(
                                   "QPushButton { color: #ccc; border: 1px solid %1; }"
                                   "QPushButton:checked { background-color: %1; color: white; border: 1px solid %1; }"
                                   "QPushButton:hover { border: 2px solid %1; }"
                                   ).arg(color);
        btn->setStyleSheet(dynamicStyle);

        if (m_selectedTags.contains(tag)) {
            btn->setChecked(true);
        }

        connect(btn, &QPushButton::clicked, this, &InspirationTagSearchDialog::onTagButtonClicked);
        m_tagButtons.append(btn);

        layout->addWidget(btn);
    }
}

void InspirationTagSearchDialog::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    refreshTagGrid();
}

void InspirationTagSearchDialog::onTagButtonClicked()
{
    QPushButton *senderBtn = qobject_cast<QPushButton*>(sender());
    if (!senderBtn) return;

    if (senderBtn->isChecked()) {
        if (m_allBtn) m_allBtn->setChecked(false);
    } else {
        bool hasChecked = false;
        for (QPushButton *btn : m_tagButtons) {
            if (btn->isChecked()) {
                hasChecked = true;
                break;
            }
        }
        if (!hasChecked && m_allBtn) m_allBtn->setChecked(true);
    }
}

void InspirationTagSearchDialog::onAllButtonClicked()
{
    if (m_allBtn->isChecked()) {
        for (QPushButton *btn : m_tagButtons) {
            btn->setChecked(false);
        }
    } else {
        m_allBtn->setChecked(true);
    }
}

void InspirationTagSearchDialog::onConfirm()
{
    m_selectedTags.clear();
    for (QPushButton *btn : m_tagButtons) {
        if (btn->isChecked()) {
            m_selectedTags.append(btn->text());
        }
    }
    accept();
}

QStringList InspirationTagSearchDialog::getSelectedTags() const
{
    return m_selectedTags;
}

bool InspirationTagSearchDialog::isMatchAll() const
{
    return m_matchAllCheckBox->isChecked();
}
