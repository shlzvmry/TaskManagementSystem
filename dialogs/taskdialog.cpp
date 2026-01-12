#include "taskdialog.h"
#include "widgets/prioritywidget.h"
#include "widgets/statuswidget.h"
#include "widgets/tagwidget.h"
#include "database/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QKeyEvent>
#include <QScrollBar>

TaskDialog::TaskDialog(QWidget *parent)
    : QDialog(parent)
    , m_isEditMode(false)
    , m_taskId(-1)
    , m_originalStatus(0)
{
    setupUI();
    setWindowTitle("创建新任务");
    setupConnections();
}

TaskDialog::TaskDialog(const QVariantMap &taskData, QWidget *parent)
    : QDialog(parent)
    , m_isEditMode(true)
    , m_taskId(taskData.value("id", -1).toInt())
    , m_originalStatus(0)
{
    setupUI();
    setWindowTitle(QString("编辑任务: %1").arg(taskData.value("title").toString()));
    setupConnections();
    populateData(taskData);
}

TaskDialog::~TaskDialog()
{
}

void TaskDialog::setupUI()
{
    resize(550, 650);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    // 1. 基本信息组
    QGroupBox *basicGroup = new QGroupBox("基本信息", this);
    QFormLayout *basicLayout = new QFormLayout(basicGroup);
    basicLayout->setSpacing(10);
    basicLayout->setLabelAlignment(Qt::AlignLeft);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("请输入任务标题");
    basicLayout->addRow("标题:", m_titleEdit);

    m_categoryCombo = new QComboBox(this);
    basicLayout->addRow("分类:", m_categoryCombo);

    m_priorityWidget = new PriorityWidget(this);
    basicLayout->addRow("优先级:", m_priorityWidget);

    m_statusWidget = new StatusWidget(this);
    basicLayout->addRow("状态:", m_statusWidget);

    // 创建时间/完成时间显示
    QWidget *timeInfoWidget = new QWidget(this);
    QHBoxLayout *timeInfoLayout = new QHBoxLayout(timeInfoWidget);
    timeInfoLayout->setContentsMargins(0, 0, 0, 0);

    m_labelCreatedTime = new QLabel("-", this);
    m_labelCompletedTime = new QLabel("-", this);
    QString timeStyle = "color: #909399; font-size: 12px;";
    m_labelCreatedTime->setStyleSheet(timeStyle);
    m_labelCompletedTime->setStyleSheet(timeStyle);

    timeInfoLayout->addWidget(new QLabel("创建于:", this));
    timeInfoLayout->addWidget(m_labelCreatedTime);

    timeInfoLayout->addStretch(1);

    timeInfoLayout->addWidget(new QLabel("完成于:", this));
    timeInfoLayout->addWidget(m_labelCompletedTime);

    timeInfoLayout->addStretch(1);

    basicLayout->addRow(timeInfoWidget);

    mainLayout->addWidget(basicGroup);

    // 2. 时间安排组
    QGroupBox *timeGroup = new QGroupBox("时间安排", this);
    QFormLayout *timeLayout = new QFormLayout(timeGroup);
    timeLayout->setSpacing(10);
    timeLayout->setLabelAlignment(Qt::AlignLeft);

    m_startEdit = new QDateTimeEdit(this);
    m_startEdit->setCalendarPopup(true);
    m_startEdit->setObjectName("dateTimeEdit");
    timeLayout->addRow("开始时间:", m_startEdit);

    m_deadlineEdit = new QDateTimeEdit(this);
    m_deadlineEdit->setCalendarPopup(true);
    m_deadlineEdit->setObjectName("dateTimeEdit");
    timeLayout->addRow("截止时间:", m_deadlineEdit);

    m_remindEdit = new QDateTimeEdit(this);
    m_remindEdit->setCalendarPopup(true);
    m_remindEdit->setObjectName("dateTimeEdit");
    timeLayout->addRow("提醒时间:", m_remindEdit);

    mainLayout->addWidget(timeGroup);

    // 3. 标签管理组
    QGroupBox *tagGroup = new QGroupBox("标签管理", this);
    QVBoxLayout *tagLayout = new QVBoxLayout(tagGroup);
    tagLayout->setSpacing(0);

    QLabel *lblSelected = new QLabel("已选标签 (点击移除):", this);
    lblSelected->setObjectName("labelSelectedTags");
    tagLayout->addWidget(lblSelected);

    QScrollArea *selectedScroll = new QScrollArea(this);
    selectedScroll->setFixedHeight(30);
    selectedScroll->setWidgetResizable(true);
    selectedScroll->setFrameShape(QFrame::NoFrame);

    QWidget *selectedContent = new QWidget();
    QHBoxLayout *selectedContentLayout = new QHBoxLayout(selectedContent);
    selectedContentLayout->setContentsMargins(0, 0, 0, 0);
    selectedContentLayout->setAlignment(Qt::AlignLeft);

    m_tagWidget = new TagWidget(this);
    selectedContentLayout->addWidget(m_tagWidget);
    selectedScroll->setWidget(selectedContent);
    tagLayout->addWidget(selectedScroll);

    QLabel *lblExisting = new QLabel("已有标签 (点击添加):", this);
    lblExisting->setObjectName("labelExistingTags");
    tagLayout->addWidget(lblExisting);

    QScrollArea *existingScroll = new QScrollArea(this);
    existingScroll->setFixedHeight(30);
    existingScroll->setWidgetResizable(true);
    existingScroll->setFrameShape(QFrame::StyledPanel); // 给个边框区分

    m_existingTagsContainer = new QWidget();
    QHBoxLayout *existingLayout = new QHBoxLayout(m_existingTagsContainer);
    existingLayout->setContentsMargins(0, 0, 0, 0);
    existingLayout->setSpacing(0);
    existingLayout->setAlignment(Qt::AlignLeft);

    existingScroll->setWidget(m_existingTagsContainer);
    tagLayout->addWidget(existingScroll);

    QHBoxLayout *addTagLayout = new QHBoxLayout();
    m_newTagEdit = new QLineEdit(this);
    m_newTagEdit->setPlaceholderText("输入新标签，用逗号分隔");
    m_newTagEdit->installEventFilter(this); // 处理回车

    QPushButton *addTagBtn = new QPushButton("添加标签", this);
    connect(addTagBtn, &QPushButton::clicked, this, &TaskDialog::onAddTagClicked);

    addTagLayout->addWidget(m_newTagEdit);
    addTagLayout->addWidget(addTagBtn);
    tagLayout->addLayout(addTagLayout);

    mainLayout->addWidget(tagGroup);

    // 4. 详细描述
    QGroupBox *descGroup = new QGroupBox("详细描述", this);
    QVBoxLayout *descLayout = new QVBoxLayout(descGroup);
    m_descEdit = new QTextEdit(this);
    m_descEdit->setPlaceholderText("请输入任务详细描述...");
    descLayout->addWidget(m_descEdit);
    mainLayout->addWidget(descGroup, 1); // 占据剩余空间

    // 5. 按钮框
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btnBox->button(QDialogButtonBox::Save)->setText("保存");
    btnBox->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(btnBox, &QDialogButtonBox::accepted, this, &TaskDialog::onSaveClicked);
    connect(btnBox, &QDialogButtonBox::rejected, this, &TaskDialog::onCancelClicked);

    mainLayout->addWidget(btnBox);

    // 初始化数据
    initDateTimeEdits();
    loadCategories();
    loadExistingTags();
}

void TaskDialog::setupConnections()
{
}

void TaskDialog::initDateTimeEdits()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime tomorrow = now.addDays(1);

    m_startEdit->setDateTime(now);
    m_startEdit->setMinimumDateTime(now.addYears(-1));
    m_startEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    m_deadlineEdit->setDateTime(tomorrow);
    m_deadlineEdit->setMinimumDateTime(now);
    m_deadlineEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    int remindMins = Database::instance().getSetting("default_remind_minutes", "60").toInt();

    if (remindMins > 0) {
        m_remindEdit->setDateTime(tomorrow.addSecs(-remindMins * 60));
    } else {
        m_remindEdit->setDateTime(tomorrow);
    }

    m_remindEdit->setMinimumDateTime(now);
    m_remindEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
}

void TaskDialog::loadCategories()
{
    m_categoryCombo->clear();
    m_categoryCombo->addItem("请选择分类", -1);

    QSqlQuery query(Database::instance().getDatabase());
    query.prepare("SELECT id, name, color FROM task_categories ORDER BY name");

    if (query.exec()) {
        while (query.next()) {
            int id = query.value("id").toInt();
            QString name = query.value("name").toString();
            QString color = query.value("color").toString();

            m_categoryCombo->addItem(name, id);
            int index = m_categoryCombo->count() - 1;

            QPixmap pixmap(16, 16);
            pixmap.fill(QColor(color));
            m_categoryCombo->setItemIcon(index, QIcon(pixmap));
        }
    }

    if (m_categoryCombo->count() > 1) {
        m_categoryCombo->setCurrentIndex(1);
    }
}

void TaskDialog::loadExistingTags()
{
    QLayout *containerLayout = m_existingTagsContainer->layout();

    QLayoutItem *item;
    while ((item = containerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    QSqlQuery query(Database::instance().getDatabase());
    query.prepare("SELECT name, color FROM task_tags ORDER BY name");

    if (query.exec()) {
        while (query.next()) {
            QString name = query.value("name").toString();
            QString color = query.value("color").toString();

            if (m_tagWidget) {
                m_tagWidget->addAvailableTag(name, color);
            }

            addExistingTagButton(name, color);
        }
    }
    static_cast<QHBoxLayout*>(containerLayout)->addStretch();
}

void TaskDialog::populateData(const QVariantMap &taskData)
{
    m_titleEdit->setText(taskData.value("title").toString());
    m_descEdit->setPlainText(taskData.value("description").toString());

    int categoryId = taskData.value("category_id").toInt();
    for (int i = 0; i < m_categoryCombo->count(); ++i) {
        if (m_categoryCombo->itemData(i).toInt() == categoryId) {
            m_categoryCombo->setCurrentIndex(i);
            break;
        }
    }

    m_priorityWidget->setPriority(taskData.value("priority", 2).toInt());

    m_originalStatus = taskData.value("status", 0).toInt();
    m_statusWidget->setStatus(m_originalStatus);

    QDateTime startTime = taskData.value("start_time").toDateTime();
    if (startTime.isValid()) m_startEdit->setDateTime(startTime);

    QDateTime deadline = taskData.value("deadline").toDateTime();
    if (deadline.isValid()) m_deadlineEdit->setDateTime(deadline);

    QDateTime remindTime = taskData.value("remind_time").toDateTime();
    if (remindTime.isValid()) m_remindEdit->setDateTime(remindTime);

    m_originalCreatedTime = taskData.value("created_at").toDateTime();
    m_labelCreatedTime->setText(m_originalCreatedTime.isValid() ? m_originalCreatedTime.toString("yyyy-MM-dd HH:mm") : "-");

    m_originalCompletedTime = taskData.value("completed_at").toDateTime();
    m_labelCompletedTime->setText(m_originalCompletedTime.isValid() ? m_originalCompletedTime.toString("yyyy-MM-dd HH:mm") : "-");

    QVariantList tagNames = taskData.value("tag_names").toList();
    QVariantList tagColors = taskData.value("tag_colors").toList();

    if (m_tagWidget && !tagNames.isEmpty()) {
        for (int i = 0; i < tagNames.size(); ++i) {
            QString tagName = tagNames.at(i).toString();
            QString tagColor = i < tagColors.size() ? tagColors.at(i).toString() : "#657896";
            m_tagWidget->addTag(tagName, tagColor);
        }
    }
}

QVariantMap TaskDialog::getTaskData() const
{
    QVariantMap taskData;
    taskData["title"] = m_titleEdit->text().trimmed();
    taskData["description"] = m_descEdit->toPlainText();
    taskData["category_id"] = m_categoryCombo->currentData().toInt();
    if (taskData["category_id"].toInt() == -1) taskData["category_id"] = 1;

    taskData["priority"] = m_priorityWidget->getPriority();

    int currentStatus = m_statusWidget->getStatus();
    taskData["status"] = currentStatus;

    if (currentStatus == 2) {
        if (m_originalStatus != 2) taskData["completed_at"] = QDateTime::currentDateTime();
        else taskData["completed_at"] = m_originalCompletedTime;
    } else {
        taskData["completed_at"] = QVariant();
    }

    if (m_isEditMode) taskData["created_at"] = m_originalCreatedTime;

    taskData["start_time"] = m_startEdit->dateTime();
    taskData["deadline"] = m_deadlineEdit->dateTime();
    taskData["remind_time"] = m_remindEdit->dateTime();
    taskData["is_reminded"] = false;
    taskData["is_deleted"] = false;

    QList<QVariantMap> tags = m_tagWidget->getTags();
    QVariantList tagNames, tagColors;
    for (const QVariantMap &tag : tags) {
        tagNames.append(tag["name"]);
        tagColors.append(tag["color"]);
    }
    taskData["tag_names"] = tagNames;
    taskData["tag_colors"] = tagColors;

    return taskData;
}


bool TaskDialog::validateInput()
{
    QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "任务标题不能为空！");
        m_titleEdit->setFocus();
        return false;
    }

    int categoryId = m_categoryCombo->currentData().toInt();
    if (categoryId == -1) {
        QMessageBox::warning(this, "输入错误", "请选择任务分类！");
        m_categoryCombo->setFocus();
        return false;
    }

    QDateTime startTime = m_startEdit->dateTime();
    QDateTime deadline = m_deadlineEdit->dateTime();

    if (startTime.isValid() && deadline.isValid() && startTime > deadline) {
        QMessageBox::warning(this, "输入错误", "开始时间不能晚于截止时间！");
        return false;
    }

    return true;
}

void TaskDialog::onSaveClicked()
{
    if (!validateInput()) {
        return;
    }

    accept();
}

void TaskDialog::onCancelClicked()
{
    reject();
}

void TaskDialog::onAddTagClicked()
{
    QString newTagText = m_newTagEdit->text().trimmed();
    if (newTagText.isEmpty()) {
        return;
    }

    QStringList newTags = newTagText.replace("，", ",").split(",", Qt::SkipEmptyParts);

    for (QString &tag : newTags) {
        tag = tag.trimmed();

        if (tag.length() > 6) {
            QMessageBox::warning(this, "格式错误", QString("标签 '%1' 过长，请限制在6个字以内").arg(tag));
            continue;
        }

        if (!tag.isEmpty()) {
            QString color = TagWidget::generateColor(tag);
            if (m_tagWidget) {
                m_tagWidget->addTag(tag, color);
            }
            if (Database::instance().addTag(tag, color)) {
                QLayout *layout = m_existingTagsContainer->layout();
                QLayoutItem *spacer = layout->takeAt(layout->count() - 1);

                addExistingTagButton(tag, color);

                if (spacer) {
                    layout->addItem(spacer);
                } else {
                    static_cast<QHBoxLayout*>(layout)->addStretch();
                }
            }
        }
    }

    m_newTagEdit->clear();
}

void TaskDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (m_titleEdit->text().isEmpty()) {
        m_titleEdit->setFocus();
    }
}

QVariantMap TaskDialog::createTask(const QVariantMap &initialData, QWidget *parent)
{
    TaskDialog dialog(parent);
    if (!initialData.isEmpty()) {
        dialog.populateData(initialData);
    }

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.getTaskData();
    }

    return QVariantMap();
}

QVariantMap TaskDialog::editTask(const QVariantMap &taskData, QWidget *parent)
{
    TaskDialog dialog(taskData, parent);

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.getTaskData();
    }

    return QVariantMap();
}

bool TaskDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_newTagEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!m_newTagEdit->text().trimmed().isEmpty()) onAddTagClicked();
            else onSaveClicked();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void TaskDialog::addExistingTagButton(const QString &name, const QString &color)
{
    QPushButton *tagBtn = new QPushButton(name, m_existingTagsContainer);
    tagBtn->setCursor(Qt::PointingHandCursor);
    tagBtn->setObjectName("existingTagBtn");
    tagBtn->setFixedHeight(20);

    QFont font = tagBtn->font();
    font.setPointSize(9);
    tagBtn->setFont(font);

    tagBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 动态样式
    QString dynamicStyle = QString(
                               "QPushButton#existingTagBtn { border: 1px solid %1; background-color: transparent; color: %1; }"
                               "QPushButton#existingTagBtn:hover { background-color: %1; color: white; }"
                               ).arg(color);
    tagBtn->setStyleSheet(dynamicStyle);

    connect(tagBtn, &QPushButton::clicked, this, [this, name, color]() {
        if (m_tagWidget) m_tagWidget->addTag(name, color);
    });

    m_existingTagsContainer->layout()->addWidget(tagBtn);
}

