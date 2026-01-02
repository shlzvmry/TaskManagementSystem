#ifndef INSPIRATIONRECYCLEBINDIALOG_H
#define INSPIRATIONRECYCLEBINDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

class InspirationModel;

class InspirationRecycleBinDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InspirationRecycleBinDialog(InspirationModel *model, QWidget *parent = nullptr);

private slots:
    void refresh();
    void onRestore();
    void onDelete();
    void onEmpty();

private:
    InspirationModel *m_model;
    QTableWidget *m_table;
    QLabel *m_statusLabel;

    void setupUI();
};

#endif // INSPIRATIONRECYCLEBINDIALOG_H
