#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include <QDateTime>
#include <QVariantMap>

class PriorityWidget;
class StatusWidget;
class TagWidget;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QScrollArea;

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
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onAddTagClicked();
    void loadCategories();
    void loadExistingTags();

private:
    // [修改] 移除 Ui::TaskDialog *ui;
    bool m_isEditMode;
    int m_taskId;

    // UI 控件指针
    QLineEdit *m_titleEdit;
    QComboBox *m_categoryCombo;
    PriorityWidget *m_priorityWidget;
    StatusWidget *m_statusWidget;

    QDateTimeEdit *m_startEdit;
    QDateTimeEdit *m_deadlineEdit;
    QDateTimeEdit *m_remindEdit;

    TagWidget *m_tagWidget;
    QWidget *m_existingTagsContainer;
    QLineEdit *m_newTagEdit;

    QTextEdit *m_descEdit;

    QLabel *m_labelCreatedTime;
    QLabel *m_labelCompletedTime;

    int m_originalStatus;
    QDateTime m_originalCompletedTime;
    QDateTime m_originalCreatedTime;

    void setupUI();
    void setupConnections();
    void initDateTimeEdits();
    void populateData(const QVariantMap &taskData);
    bool validateInput();
    void addExistingTagButton(const QString &name, const QString &color);
};

#endif // TASKDIALOG_H
