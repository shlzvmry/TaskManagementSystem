#ifndef TASKTABLEVIEW_H
#define TASKTABLEVIEW_H

#include <QTableView>

class TaskTableView : public QTableView
{
    Q_OBJECT

public:
    explicit TaskTableView(QWidget *parent = nullptr);

    // 重写 setModel 以便在设置模型后应用列宽和代理
    void setModel(QAbstractItemModel *model) override;

signals:
    // 发送编辑任务请求信号
    void editTaskRequested(int taskId);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    void setupUI();
};

#endif // TASKTABLEVIEW_H
