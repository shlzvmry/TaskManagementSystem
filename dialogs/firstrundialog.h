#ifndef FIRSTRUNDIALOG_H
#define FIRSTRUNDIALOG_H

#include <QDialog>

class QRadioButton;
class QListWidget;

class FirstRunDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FirstRunDialog(QWidget *parent = nullptr);

private slots:
    void onTypeChanged();
    void onConfirm();

private:
    QRadioButton *m_studentBtn;
    QRadioButton *m_workerBtn;
    QRadioButton *m_customBtn;
    QListWidget *m_categoryList;

    void setupUI();
    void loadDefaults(const QStringList &categories, const QStringList &colors);
};

#endif // FIRSTRUNDIALOG_H
