#include "tagwidget.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>
#include <QCryptographicHash>

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
{
    setupUI();
}

void TagWidget::setupUI()
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);
    m_layout->setAlignment(Qt::AlignLeft);

    // 设置固定高度
    setFixedHeight(40);

    // 允许换行
    m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
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
        tag["color"] = button->property("color").toString();
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
    QPushButton *button = new QPushButton(name, this);
    button->setProperty("color", color);

    // 查找预设颜色
    QString finalColor = color;
    for (const QVariantMap &tag : m_availableTags) {
        if (tag["name"].toString() == name) {
            finalColor = tag["color"].toString();
            break;
        }
    }

    // 如果没有指定颜色，生成一个
    if (finalColor == "#657896") {
        finalColor = generateColor(name);
    }

    // 根据颜色亮度决定文字颜色
    QColor bgColor(finalColor);
    QString textColor = bgColor.lightness() > 128 ? "black" : "white";

    QString style = QString(
                        "QPushButton {"
                        "    background-color: %1;"
                        "    color: %2;"
                        "    border: none;"
                        "    border-radius: 12px;"
                        "    padding: 4px 12px;"
                        "    font-size: 11px;"
                        "    font-weight: normal;"
                        "}"
                        "QPushButton:hover {"
                        "    background-color: %3;"
                        "}"
                        ).arg(bgColor.name(),
                             textColor,
                             bgColor.lighter(120).name());

    button->setStyleSheet(style);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedHeight(24);

    // 设置工具提示
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
    // 强制更新布局
    m_layout->update();
    update();
}

void TagWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // 如果需要，可以绘制背景
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
}

QString TagWidget::generateColor(const QString &text)
{
    // 根据文本生成一致的颜色
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(text.toUtf8());
    QByteArray result = hash.result();

    // 从hash中提取颜色分量
    int r = static_cast<unsigned char>(result[0]) % 200 + 55;
    int g = static_cast<unsigned char>(result[1]) % 200 + 55;
    int b = static_cast<unsigned char>(result[2]) % 200 + 55;

    return QColor(r, g, b).name();
}
