#ifndef INSPIRATIONDIALOG_H
#define INSPIRATIONDIALOG_H

#include <QDialog>
#include <QVariantMap>

class QTextEdit;
class QLineEdit;

class InspirationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InspirationDialog(QWidget *parent = nullptr);
    explicit InspirationDialog(const QVariantMap &data, QWidget *parent = nullptr);

    QVariantMap getData() const;

private slots:
    void onSave();

private:
    QTextEdit *m_contentEdit;
    QLineEdit *m_tagsEdit;
    int m_id;

    void setupUI();
    void populateData(const QVariantMap &data);
};

#endif // INSPIRATIONDIALOG_H
