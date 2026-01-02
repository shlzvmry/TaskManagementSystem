#ifndef TASKTABLEVIEW_H
#define TASKTABLEVIEW_H

#include <QTableView>

class TaskTableView : public QTableView
{
    Q_OBJECT

public:
    explicit TaskTableView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

signals:
    void editTaskRequested(int taskId);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    void setupUI();
};

#endif // TASKTABLEVIEW_H
