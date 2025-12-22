#ifndef RECYCLEBINDIALOG_H
#define RECYCLEBINDIALOG_H

#include <QDialog>
#include <QList>
#include <QVariantMap>

namespace Ui {
class RecycleBinDialog;
}

class TaskModel;

class RecycleBinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecycleBinDialog(QWidget *parent = nullptr);
    ~RecycleBinDialog();

    void setTaskModel(TaskModel *model);
    void refreshDeletedTasks();

private slots:
    void onRestoreClicked();
    void onDeletePermanentlyClicked();
    void onClearAllClicked();
    void onRefreshClicked();
    void onCloseClicked();
    void updateButtonStates();

private:
    Ui::RecycleBinDialog *ui;
    TaskModel *taskModel;
    QList<QVariantMap> deletedTasks;

    void setupUI();
    void setupConnections();
    void setupTable();
    int getSelectedTaskId() const;
    void showConfirmationDialog(const QString &title, const QString &message,
                                std::function<void()> onConfirmed);
};

#endif // RECYCLEBINDIALOG_H
