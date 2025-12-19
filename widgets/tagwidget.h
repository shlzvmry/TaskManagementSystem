#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
#include <QList>
#include <QVariantMap>

class QHBoxLayout;
class QPushButton;

class TagWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TagWidget(QWidget *parent = nullptr);

    void addTag(const QString &name, const QString &color = "#657896");
    void addAvailableTag(const QString &name, const QString &color = "#657896");
    void removeTag(const QString &name);
    void clearTags();

    QList<QVariantMap> getTags() const;
    bool hasTag(const QString &name) const;

    static QString generateColor(const QString &text);

signals:
    void tagAdded(const QString &name);
    void tagRemoved(const QString &name);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onTagButtonClicked();

private:
    QHBoxLayout *m_layout;
    QList<QPushButton*> m_tagButtons;
    QList<QVariantMap> m_availableTags;

    void setupUI();
    QPushButton* createTagButton(const QString &name, const QString &color);
    void updateLayout();
};

#endif // TAGWIDGET_H
