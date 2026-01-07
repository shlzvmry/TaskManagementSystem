#ifndef TAGMANAGERDIALOG_H
#define TAGMANAGERDIALOG_H

#include <QDialog>
#include <QVariantMap>

class QListWidget;
class QTableWidget;
class QLineEdit;
class QListWidgetItem;

class TagManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagManagerDialog(QWidget *parent = nullptr);
    ~TagManagerDialog();

private slots:
    void onAddTagClicked();
    void onDeleteTagClicked();
    void onRemoveRelationClicked();
    void onTagSelected(QListWidgetItem *item);
    void refreshTags();

private:
    QList<QVariantMap> allTags;
    QListWidget *m_tagListWidget;
    QTableWidget *m_taskTableWidget;
    QLineEdit *m_tagNameInput;

    void setupUI();
};

#endif // TAGMANAGERDIALOG_H
