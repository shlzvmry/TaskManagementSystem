#ifndef INSPIRATIONTAGSEARCHDIALOG_H
#define INSPIRATIONTAGSEARCHDIALOG_H

#include <QDialog>
#include <QList>
#include <QStringList>

class QLineEdit;
class QCheckBox;
class QScrollArea;
class QPushButton;
class InspirationModel;

class InspirationTagSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InspirationTagSearchDialog(InspirationModel *model, const QStringList &initialSelection, bool initialMatchAll,QWidget *parent = nullptr);

    QStringList getSelectedTags() const;
    bool isMatchAll() const;

private slots:
    void onSearchTextChanged(const QString &text);
    void onTagButtonClicked();
    void onAllButtonClicked();
    void onConfirm();

private:
    InspirationModel *m_model;
    QLineEdit *m_searchEdit;
    QScrollArea *m_scrollArea;
    QWidget *m_containerWidget;
    QCheckBox *m_matchAllCheckBox;
    QPushButton *m_allBtn;

    QList<QPushButton*> m_tagButtons;
    QStringList m_selectedTags;
    QStringList m_allTags;

    void setupUI();
    void loadTags();
    void refreshTagGrid();
    void updateAllButtonState();
    bool m_initialMatchAll;
};

#endif // INSPIRATIONTAGSEARCHDIALOG_H
