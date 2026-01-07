#ifndef RECYCLEBINDIALOG_H
#define RECYCLEBINDIALOG_H

#include <QDialog>
#include <QList>
#include <QVariantMap>
#include <functional>

class TaskModel;
class QTableWidget;
class QLabel;
class QPushButton;

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
    TaskModel *taskModel;
    QList<QVariantMap> deletedTasks;

    QLabel *m_statusLabel;
    QTableWidget *m_tableWidget;
    QPushButton *m_restoreBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_clearBtn;

    void setupUI();
    void setupTable();
    int getSelectedTaskId() const;
    void showConfirmationDialog(const QString &title, const QString &message,
                                std::function<void()> onConfirmed);
};

#endif // RECYCLEBINDIALOG_H
