#ifndef KANBANVIEW_H
#define KANBANVIEW_H

#include <QWidget>
#include <QListView>
#include <QStyledItemDelegate>

class TaskModel;
class TaskFilterModel;
class QHBoxLayout;

class KanbanDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit KanbanDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class KanbanColumn : public QListView
{
    Q_OBJECT
public:
    explicit KanbanColumn(int value, QWidget *parent = nullptr);
    int getValue() const { return m_value; }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;

signals:
    void taskDropped(int taskId, int newValue);
    void taskDoubleClicked(int taskId);

private:
    int m_value;
};

class KanbanView : public QWidget
{
    Q_OBJECT
public:
    enum GroupMode { GroupByStatus, GroupByPriority };
    void setFilter(int categoryId, int priority, const QString &text);
    explicit KanbanView(QWidget *parent = nullptr);
    void setModel(TaskModel *model);
    void setGroupMode(GroupMode mode);
    GroupMode getGroupMode() const { return m_groupMode; }

signals:
    void editTaskRequested(int taskId);

private:
    TaskModel *m_model;
    class QHBoxLayout *m_columnLayout;
    QList<KanbanColumn*> m_columns;
    QList<TaskFilterModel*> m_filters;
    GroupMode m_groupMode;

    void setupUI();
    KanbanColumn* createColumn(const QString &title, int value, const QString &color);
    void refreshColumns();
};

#endif // KANBANVIEW_H
