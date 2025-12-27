#include "taskdialog.h"
#include "ui_taskdialog.h"
#include "widgets/prioritywidget.h"
#include "widgets/statuswidget.h"
#include "widgets/tagwidget.h"
#include "database/database.h"

#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QOverload>
#include <QScrollArea>
#include <QHBoxLayout>

TaskDialog::TaskDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TaskDialog)
    , m_isEditMode(false)
    , m_taskId(-1)
    , m_priorityWidget(nullptr)
    , m_statusWidget(nullptr)
    , m_tagWidget(nullptr)
    , m_existingTagsContainer(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("创建新任务");
    setupUI();
    setupConnections();
}

TaskDialog::TaskDialog(const QVariantMap &taskData, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TaskDialog)
    , m_isEditMode(true)
    , m_taskId(taskData.value("id", -1).toInt())
    , m_priorityWidget(nullptr)
    , m_statusWidget(nullptr)
    , m_tagWidget(nullptr)
    , m_existingTagsContainer(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(QString("编辑任务: %1").arg(taskData.value("title").toString()));
    setupUI();
    setupConnections();
    populateData(taskData);
}

TaskDialog::~TaskDialog()
{
    delete ui;
}

void TaskDialog::setupUI()
{
    // 设置对话框属性
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 初始化优先级和状态控件
    m_priorityWidget = new PriorityWidget(this);
    m_statusWidget = new StatusWidget(this);

    ui->widgetPriority->layout()->addWidget(m_priorityWidget);
    ui->widgetStatus->layout()->addWidget(m_statusWidget);

    // 初始化标签控件
    m_tagWidget = new TagWidget(this);
    ui->widgetTags->layout()->addWidget(m_tagWidget);

    // 初始化时间控件
    initDateTimeEdits();

    // 加载分类
    loadCategories();

    // 加载现有标签
    loadExistingTags();

    // 设置按钮文本
    ui->buttonBox->button(QDialogButtonBox::Save)->setText("保存");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
}

void TaskDialog::setupConnections()
{
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TaskDialog::onSaveClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &TaskDialog::onCancelClicked);
    connect(ui->pushButtonAddTag, &QPushButton::clicked, this, &TaskDialog::onAddTagClicked);

    if (m_tagWidget) {
        connect(m_tagWidget, &TagWidget::tagRemoved, this, &TaskDialog::onTagRemoved);
    }
}

void TaskDialog::initDateTimeEdits()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime tomorrow = now.addDays(1);

    // 设置开始时间（默认为现在）
    ui->dateTimeEditStart->setDateTime(now);
    ui->dateTimeEditStart->setMinimumDateTime(now.addYears(-1));
    ui->dateTimeEditStart->setDisplayFormat("yyyy-MM-dd HH:mm");

    // 设置截止时间（默认为明天）
    ui->dateTimeEditDeadline->setDateTime(tomorrow);
    ui->dateTimeEditDeadline->setMinimumDateTime(now);
    ui->dateTimeEditDeadline->setDisplayFormat("yyyy-MM-dd HH:mm");

    // 设置提醒时间（默认为截止时间前1小时）
    ui->dateTimeEditRemind->setDateTime(tomorrow.addSecs(-3600));
    ui->dateTimeEditRemind->setMinimumDateTime(now);
    ui->dateTimeEditRemind->setDisplayFormat("yyyy-MM-dd HH:mm");
}

void TaskDialog::loadCategories()
{
    ui->comboBoxCategory->clear();
    ui->comboBoxCategory->addItem("请选择分类", -1);

    QSqlQuery query(Database::instance().getDatabase());
    query.prepare("SELECT id, name, color FROM task_categories ORDER BY name");

    if (query.exec()) {
        while (query.next()) {
            int id = query.value("id").toInt();
            QString name = query.value("name").toString();
            QString color = query.value("color").toString();

            // 设置项目显示文本和颜色
            ui->comboBoxCategory->addItem(name, id);
            int index = ui->comboBoxCategory->count() - 1;

            // 为分类项设置颜色（可选）
            QPixmap pixmap(16, 16);
            pixmap.fill(QColor(color));
            ui->comboBoxCategory->setItemIcon(index, QIcon(pixmap));
        }
    }

    // 默认选择第一个内置分类
    if (ui->comboBoxCategory->count() > 1) {
        ui->comboBoxCategory->setCurrentIndex(1);
    }
}

void TaskDialog::loadExistingTags()
{
    // 1. 清理旧的容器（如果有）
    if (m_existingTagsContainer) {
        m_existingTagsContainer->deleteLater();
        m_existingTagsContainer = nullptr;
    }

    // 2. 创建滚动区域来放置标签按钮
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFixedHeight(50);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; } QWidget { background: transparent; }");

    m_existingTagsContainer = new QWidget(scrollArea);
    QHBoxLayout *containerLayout = new QHBoxLayout(m_existingTagsContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(8);
    containerLayout->setAlignment(Qt::AlignLeft);

    scrollArea->setWidget(m_existingTagsContainer);

    // 将滚动区域添加到界面
    QVBoxLayout *tagsLayout = qobject_cast<QVBoxLayout*>(ui->groupBoxTags->layout());
    if (tagsLayout) {
        tagsLayout->insertWidget(1, scrollArea);
    }

    // 3. 从数据库加载标签并创建按钮
    QSqlQuery query(Database::instance().getDatabase());
    query.prepare("SELECT name, color FROM task_tags ORDER BY name");

    if (query.exec()) {
        while (query.next()) {
            QString name = query.value("name").toString();
            QString color = query.value("color").toString();

            if (m_tagWidget) {
                m_tagWidget->addAvailableTag(name, color);
            }

            QPushButton *tagBtn = new QPushButton(name, m_existingTagsContainer);
            tagBtn->setCursor(Qt::PointingHandCursor);
            tagBtn->setObjectName("existingTagBtn");
            tagBtn->setFixedSize(72, 15);

            // 修改圆角为 4px，与 TagWidget 和 PriorityWidget 保持一致
            QString dynamicStyle = QString(
                                       "QPushButton#existingTagBtn {"
                                       "  background-color: transparent;"
                                       "  color: #CCCCCC;"
                                       "  border: 1px solid %1;"
                                       "  border-radius: 4px;"
                                       "}"
                                       "QPushButton#existingTagBtn:hover {"
                                       "  background-color: %1;"
                                       "  color: white;"
                                       "  border: 1px solid %1;"
                                       "}"
                                       ).arg(color);

            tagBtn->setStyleSheet(dynamicStyle);

            connect(tagBtn, &QPushButton::clicked, this, [this, name, color]() {
                if (m_tagWidget) {
                    m_tagWidget->addTag(name, color);
                }
            });

            containerLayout->addWidget(tagBtn);
        }
    }
}

void TaskDialog::populateData(const QVariantMap &taskData)
{
    // 填充基本信息
    ui->lineEditTitle->setText(taskData.value("title").toString());
    ui->textEditDescription->setPlainText(taskData.value("description").toString());

    // 填充分类
    int categoryId = taskData.value("category_id").toInt();
    for (int i = 0; i < ui->comboBoxCategory->count(); ++i) {
        if (ui->comboBoxCategory->itemData(i).toInt() == categoryId) {
            ui->comboBoxCategory->setCurrentIndex(i);
            break;
        }
    }

    // 填充优先级和状态
    if (m_priorityWidget) {
        m_priorityWidget->setPriority(taskData.value("priority", 2).toInt());
    }

    if (m_statusWidget) {
        m_statusWidget->setStatus(taskData.value("status", 0).toInt());
    }

    // 填充时间
    QDateTime startTime = taskData.value("start_time").toDateTime();
    if (startTime.isValid()) {
        ui->dateTimeEditStart->setDateTime(startTime);
    }

    QDateTime deadline = taskData.value("deadline").toDateTime();
    if (deadline.isValid()) {
        ui->dateTimeEditDeadline->setDateTime(deadline);
    }

    QDateTime remindTime = taskData.value("remind_time").toDateTime();
    if (remindTime.isValid()) {
        ui->dateTimeEditRemind->setDateTime(remindTime);
    }

    // 填充标签
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

    // 基本信息
    taskData["title"] = ui->lineEditTitle->text().trimmed();
    taskData["description"] = ui->textEditDescription->toPlainText();
    taskData["category_id"] = ui->comboBoxCategory->currentData().toInt();

    // 如果未选择分类，使用默认值
    if (taskData["category_id"].toInt() == -1) {
        taskData["category_id"] = 1; // 默认使用第一个分类
    }

    // 优先级和状态
    if (m_priorityWidget) {
        taskData["priority"] = m_priorityWidget->getPriority();
    } else {
        taskData["priority"] = 2; // 默认普通优先级
    }

    if (m_statusWidget) {
        taskData["status"] = m_statusWidget->getStatus();
    } else {
        taskData["status"] = 0; // 默认待办状态
    }

    // 时间信息
    taskData["start_time"] = ui->dateTimeEditStart->dateTime();
    taskData["deadline"] = ui->dateTimeEditDeadline->dateTime();
    taskData["remind_time"] = ui->dateTimeEditRemind->dateTime();

    // 其他字段
    taskData["is_reminded"] = false;
    taskData["is_deleted"] = false;
    taskData["completed_at"] = QVariant(); // 空时间

    // 标签信息
    if (m_tagWidget) {
        QList<QVariantMap> tags = m_tagWidget->getTags();
        QVariantList tagNames, tagColors;

        for (const QVariantMap &tag : tags) {
            tagNames.append(tag["name"]);
            tagColors.append(tag["color"]);
        }

        taskData["tag_names"] = tagNames;
        taskData["tag_colors"] = tagColors;
    }

    qDebug() << "TaskDialog: 准备提交的任务数据:" << taskData;

    return taskData;
}

bool TaskDialog::validateInput()
{
    QString title = ui->lineEditTitle->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "任务标题不能为空！");
        ui->lineEditTitle->setFocus();
        return false;
    }

    int categoryId = ui->comboBoxCategory->currentData().toInt();
    if (categoryId == -1) {
        QMessageBox::warning(this, "输入错误", "请选择任务分类！");
        ui->comboBoxCategory->setFocus();
        return false;
    }

    QDateTime startTime = ui->dateTimeEditStart->dateTime();
    QDateTime deadline = ui->dateTimeEditDeadline->dateTime();

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
    QString newTagText = ui->lineEditNewTag->text().trimmed();
    if (newTagText.isEmpty()) {
        return;
    }

    // 支持逗号分隔的多个标签 - Qt6使用Qt::SplitBehavior
    QStringList newTags = newTagText.split(",", Qt::SkipEmptyParts);

    for (QString &tag : newTags) {
        tag = tag.trimmed();
        if (!tag.isEmpty() && m_tagWidget) {
            // 使用主题色作为新标签颜色
            m_tagWidget->addTag(tag, "#657896");
        }
    }

    ui->lineEditNewTag->clear();
}

void TaskDialog::onTagRemoved(const QString &tagName)
{
    qDebug() << "标签被移除:" << tagName;
}

void TaskDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // 设置焦点
    if (ui->lineEditTitle->text().isEmpty()) {
        ui->lineEditTitle->setFocus();
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
