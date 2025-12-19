#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include <QDateTime>
#include <QVariantMap>

namespace Ui {
class TaskDialog;
}

class PriorityWidget;
class StatusWidget;
class TagWidget;

class TaskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TaskDialog(QWidget *parent = nullptr);
    explicit TaskDialog(const QVariantMap &taskData, QWidget *parent = nullptr);
    ~TaskDialog();

    QVariantMap getTaskData() const;
    bool isEditMode() const { return m_isEditMode; }
    int getTaskId() const { return m_taskId; }

    static QVariantMap createTask(const QVariantMap &initialData = QVariantMap(),
                                  QWidget *parent = nullptr);
    static QVariantMap editTask(const QVariantMap &taskData, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onAddTagClicked();
    void onTagRemoved(const QString &tagName);
    void loadCategories();
    void loadExistingTags();

private:
    Ui::TaskDialog *ui;
    bool m_isEditMode;
    int m_taskId;

    PriorityWidget *m_priorityWidget;
    StatusWidget *m_statusWidget;
    TagWidget *m_tagWidget;

    void setupUI();
    void setupConnections();
    void initDateTimeEdits();
    void populateData(const QVariantMap &taskData);
    bool validateInput();
    QList<QVariantMap> getSelectedTags() const;
};

#endif // TASKDIALOG_H
