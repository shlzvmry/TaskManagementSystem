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
#include <QScrollBar>
#include <QWheelEvent>

TaskDialog::TaskDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TaskDialog)
    , m_isEditMode(false)
    , m_taskId(-1)
    , m_priorityWidget(nullptr)
    , m_statusWidget(nullptr)
    , m_tagWidget(nullptr)
    , m_existingTagsContainer(nullptr)
    , m_labelCreatedTime(nullptr)
    , m_labelCompletedTime(nullptr)
    , m_originalStatus(0)
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
    , m_labelCreatedTime(nullptr)
    , m_labelCompletedTime(nullptr)
    , m_originalStatus(0)
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

    ui->textEditDescription->setMaximumHeight(16777215);
    ui->textEditDescription->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    this->resize(500, 580);

    // 初始化优先级和状态控件
    m_priorityWidget = new PriorityWidget(this);
    m_statusWidget = new StatusWidget(this);

    ui->widgetPriority->layout()->addWidget(m_priorityWidget);
    ui->widgetStatus->layout()->addWidget(m_statusWidget);

    // 初始化标签控件
    m_tagWidget = new TagWidget(this);
    ui->widgetTags->layout()->addWidget(m_tagWidget);

    // 安装事件过滤器以支持横向滚动
    ui->scrollAreaSelectedTags->installEventFilter(this);
    ui->scrollAreaExistingTags->installEventFilter(this);
    ui->lineEditNewTag->installEventFilter(this);
    ui->scrollAreaWidgetContentsExisting->layout()->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // 初始化时间控件
    initDateTimeEdits();

    // 加载分类
    loadCategories();

    // 加载现有标签
    loadExistingTags();

    // 设置按钮文本
    ui->buttonBox->button(QDialogButtonBox::Save)->setText("保存");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");

    // --- 动态添加时间显示标签 ---
    QFormLayout *formLayout = qobject_cast<QFormLayout*>(ui->groupBoxBasic->layout());
    if (formLayout) {
        m_labelCreatedTime = new QLabel("-", this);
        m_labelCompletedTime = new QLabel("-", this);

        QString style = "color: #888888; font-weight: bold;";
        m_labelCreatedTime->setStyleSheet("color: #657896;");
        m_labelCompletedTime->setStyleSheet("color: #657896;");
        QWidget *timeContainer = new QWidget(this);
        QHBoxLayout *timeLayout = new QHBoxLayout(timeContainer);
        timeLayout->setContentsMargins(0, 0, 0, 0);
        timeLayout->setSpacing(10);
        timeLayout->addWidget(new QLabel("创建于:", this));
        timeLayout->addWidget(m_labelCreatedTime);
        timeLayout->addStretch(1);
        timeLayout->addWidget(new QLabel("完成于:", this));
        timeLayout->addWidget(m_labelCompletedTime);
        timeLayout->addStretch(1);
        formLayout->addRow(timeContainer);
    }
}

void TaskDialog::setupConnections()
{
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TaskDialog::onSaveClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &TaskDialog::onCancelClicked);
    connect(ui->pushButtonAddTag, &QPushButton::clicked, this, &TaskDialog::onAddTagClicked);
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

            // 为分类项设置颜色
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
    // 获取 UI 中定义的容器布局
    QLayout *containerLayout = ui->scrollAreaWidgetContentsExisting->layout();

    // 清除旧的按钮
    QLayoutItem *item;
    while ((item = containerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    //从数据库加载标签并创建按钮
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
        m_originalStatus = taskData.value("status", 0).toInt();
        m_statusWidget->setStatus(m_originalStatus);
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
    m_originalCreatedTime = taskData.value("created_at").toDateTime();
    if (m_labelCreatedTime) {
        m_labelCreatedTime->setText(m_originalCreatedTime.isValid() ?
                                        m_originalCreatedTime.toString("yyyy-MM-dd HH:mm:ss") : "-");
    }

    m_originalCompletedTime = taskData.value("completed_at").toDateTime();
    if (m_labelCompletedTime) {
        if (m_originalCompletedTime.isValid()) {
            m_labelCompletedTime->setText(m_originalCompletedTime.toString("yyyy-MM-dd HH:mm:ss"));
        } else {
            m_labelCompletedTime->setText("-");
        }
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

    if (taskData["category_id"].toInt() == -1) {
        taskData["category_id"] = 1;
    }

    // 优先级
    if (m_priorityWidget) {
        taskData["priority"] = m_priorityWidget->getPriority();
    } else {
        taskData["priority"] = 2;
    }

    // 状态与完成时间逻辑
    int currentStatus = 0;
    if (m_statusWidget) {
        currentStatus = m_statusWidget->getStatus();
        taskData["status"] = currentStatus;
    }

    if (currentStatus == 2) { // 如果当前是"已完成"
        if (m_originalStatus != 2) {
            // 状态从"非已完成"变为"已完成" -> 更新为当前时间
            taskData["completed_at"] = QDateTime::currentDateTime();
        } else {
            // 状态一直是"已完成" -> 保持原有完成时间
            taskData["completed_at"] = m_originalCompletedTime;
        }
    } else {
        // 如果当前不是"已完成" -> 清空完成时间
        taskData["completed_at"] = QVariant();
    }

    // 保持原有的创建时间（如果是编辑模式）
    if (m_isEditMode) {
        taskData["created_at"] = m_originalCreatedTime;
    }

    // 时间信息
    taskData["start_time"] = ui->dateTimeEditStart->dateTime();
    taskData["deadline"] = ui->dateTimeEditDeadline->dateTime();
    taskData["remind_time"] = ui->dateTimeEditRemind->dateTime();

    taskData["is_reminded"] = false;
    taskData["is_deleted"] = false;

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
                QLayout *layout = ui->scrollAreaWidgetContentsExisting->layout();
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

    ui->lineEditNewTag->clear();
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

bool TaskDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->lineEditNewTag && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // 输入框有内容 -> 添加标签，不关闭窗口
            if (!ui->lineEditNewTag->text().trimmed().isEmpty()) {
                onAddTagClicked();
            }
            // 输入框为空 -> 执行保存操作
            else {
                onSaveClicked();
            }
            return true;
        }
    }

    // 处理横向滚动
    if (event->type() == QEvent::Wheel) {
        QScrollArea *scrollArea = qobject_cast<QScrollArea*>(obj);
        if (scrollArea && (scrollArea == ui->scrollAreaSelectedTags || scrollArea == ui->scrollAreaExistingTags)) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

            int delta = wheelEvent->angleDelta().y();

            if (delta != 0) {
                QScrollBar *bar = scrollArea->horizontalScrollBar();
                bar->setValue(bar->value() - delta);
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void TaskDialog::addExistingTagButton(const QString &name, const QString &color)
{
    QPushButton *tagBtn = new QPushButton(name, ui->scrollAreaWidgetContentsExisting);
    tagBtn->setCursor(Qt::PointingHandCursor);
    tagBtn->setObjectName("existingTagBtn");

    tagBtn->setFixedHeight(24);

    QFont font = tagBtn->font();
    if (font.pointSize() < 9) font.setPointSize(9);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(name);
    int padding = 24;
    tagBtn->setFixedWidth(textWidth + padding);

    QString dynamicStyle = QString(
                               "QPushButton#existingTagBtn {"
                               "  border: 1px solid %1;"
                               "}"
                               "QPushButton#existingTagBtn:hover {"
                               "  background-color: %1;"
                               "  border: 1px solid %1;"
                               "}"
                               ).arg(color);

    tagBtn->setStyleSheet(dynamicStyle);

    connect(tagBtn, &QPushButton::clicked, this, [this, name, color]() {
        if (m_tagWidget) {
            m_tagWidget->addTag(name, color);
        }
    });

    ui->scrollAreaWidgetContentsExisting->layout()->addWidget(tagBtn);
}
