#ifndef TAGMANAGERDIALOG_H
#define TAGMANAGERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVariantMap>

namespace Ui {
class TagManagerDialog;
}

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
    Ui::TagManagerDialog *ui;
    QList<QVariantMap> allTags;

    void setupUI();
    void setupConnections();
};

#endif // TAGMANAGERDIALOG_H
