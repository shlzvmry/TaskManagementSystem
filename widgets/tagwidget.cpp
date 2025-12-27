#include "tagwidget.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>
#include <QCryptographicHash>
#include <QStyleOptionButton>

// 自定义标签按钮
class TagButton : public QPushButton {
public:
    TagButton(const QString &text, QWidget *parent = nullptr) : QPushButton(text, parent) {
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

protected:
    void enterEvent(QEnterEvent *event) override {
        m_isHovered = true;
        update();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        m_isHovered = false;
        update();
        QPushButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor bgColor = property("tagColor").value<QColor>();
        if (!bgColor.isValid()) bgColor = QColor("#657896");

        if (m_isHovered) {
            bgColor = QColor("#FF6B6B");
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawRoundedRect(rect(), 4, 4);

        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);

        painter.drawText(rect(), Qt::AlignCenter, text());
    }

private:
    bool m_isHovered = false;
};

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
{
    setupUI();
}

void TagWidget::setupUI()
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
    m_layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // 增加垂直居中
    setFixedHeight(30);
}

void TagWidget::addTag(const QString &name, const QString &color)
{
    if (name.isEmpty() || hasTag(name)) {
        return;
    }

    QPushButton *tagButton = createTagButton(name, color);
    m_tagButtons.append(tagButton);
    m_layout->addWidget(tagButton);

    emit tagAdded(name);
}

void TagWidget::addAvailableTag(const QString &name, const QString &color)
{
    QVariantMap tag;
    tag["name"] = name;
    tag["color"] = color;
    m_availableTags.append(tag);
}

void TagWidget::removeTag(const QString &name)
{
    for (int i = 0; i < m_tagButtons.size(); ++i) {
        if (m_tagButtons[i]->text() == name) {
            QPushButton *button = m_tagButtons.takeAt(i);
            m_layout->removeWidget(button);
            button->deleteLater();
            emit tagRemoved(name);
            break;
        }
    }
}

void TagWidget::clearTags()
{
    for (QPushButton *button : m_tagButtons) {
        m_layout->removeWidget(button);
        button->deleteLater();
    }
    m_tagButtons.clear();
}

QList<QVariantMap> TagWidget::getTags() const
{
    QList<QVariantMap> tags;
    for (QPushButton *button : m_tagButtons) {
        QVariantMap tag;
        tag["name"] = button->text();
        tag["color"] = button->property("tagColor").value<QColor>().name();
        tags.append(tag);
    }
    return tags;
}

bool TagWidget::hasTag(const QString &name) const
{
    for (QPushButton *button : m_tagButtons) {
        if (button->text() == name) {
            return true;
        }
    }
    return false;
}

QPushButton* TagWidget::createTagButton(const QString &name, const QString &color)
{
    TagButton *button = new TagButton(name, this);

    QString finalColor = color;
    for (const QVariantMap &tag : m_availableTags) {
        if (tag["name"].toString() == name) {
            finalColor = tag["color"].toString();
            break;
        }
    }

    if (finalColor == "#657896") {
        finalColor = generateColor(name);
    }

    button->setProperty("tagColor", QColor(finalColor));
    button->setFixedHeight(26);

    QFont font = button->font();
    font.setPointSize(10);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(name);
    int padding = 24;
    button->setFixedWidth(textWidth + padding);

    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(QString("点击移除标签: %1").arg(name));

    connect(button, &QPushButton::clicked, this, &TagWidget::onTagButtonClicked);

    return button;
}

void TagWidget::onTagButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (button) {
        removeTag(button->text());
    }
}

void TagWidget::updateLayout()
{
    m_layout->update();
    update();
}

void TagWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
}

QString TagWidget::generateColor(const QString &text)
{
    // 预定义莫兰迪色盘
    static const QStringList palette = {
        "#C96A6A", // 柔和砖红
        "#D69E68", // 大地橙
        "#7FA882", // 鼠尾草绿
        "#8C949E", // 冷灰
        "#7696B3", // 钢蓝
        "#B48EAD", // 莫兰迪紫
        "#88C0D0", // 北欧蓝
        "#81A1C1", // 冰河蓝
        "#BF616A", // 浆果红
        "#EBCB8B"  // 麦穗黄
    };

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(text.toUtf8());
    QByteArray result = hash.result();

    // 使用哈希值的第一个字节作为索引，从色盘中取色
    unsigned char index = static_cast<unsigned char>(result[0]);
    return palette[index % palette.size()];
}
