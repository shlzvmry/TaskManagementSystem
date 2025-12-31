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

// 单个看板列
class KanbanColumn : public QListView
{
    Q_OBJECT
public:
    explicit KanbanColumn(int status, QWidget *parent = nullptr);
    int getStatus() const { return m_status; }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void taskDropped(int taskId, int newStatus);

private:
    int m_status;
};

// 看板主视图
class KanbanView : public QWidget
{
    Q_OBJECT
public:
    explicit KanbanView(QWidget *parent = nullptr);
    void setModel(TaskModel *model);

private:
    TaskModel *m_model;
    QHBoxLayout *m_layout;
    QList<KanbanColumn*> m_columns;
    QList<TaskFilterModel*> m_filters;

    void setupUI();
    KanbanColumn* createColumn(const QString &title, int status, const QString &color);
};

#endif // KANBANVIEW_H
