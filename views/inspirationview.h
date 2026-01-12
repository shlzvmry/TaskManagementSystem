#ifndef INSPIRATIONVIEW_H
#define INSPIRATIONVIEW_H

#include <QWidget>
#include <QStyledItemDelegate>
#include <QListWidget>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QDate>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>

class InspirationModel;
class QTableView;
class QLineEdit;
class QSpinBox;
class QCheckBox;

class InspirationGridDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit InspirationGridDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class CalendarView;

class InspirationView : public QWidget
{
    Q_OBJECT

public:
    explicit InspirationView(QWidget *parent = nullptr);
    void setModel(InspirationModel *model);
    void setFirstDayOfWeek(Qt::DayOfWeek dayOfWeek);
    bool restoreInspiration(int id);
    bool permanentDeleteInspiration(int id);
    QList<QVariantMap> getDeletedInspirations() const;
    bool emptyRecycleBin();
    bool renameTag(const QString &oldName, const QString &newName);
    bool removeTagFromAll(const QString &tagName);

public slots:
    void refresh();
    void setTaskModel(class TaskModel *model);

signals:
    void showRecycleBinRequested();
    void showTagManagerRequested();
    void showInspirationsRequested(const QDate &date);
    void showTasksRequested(const QDate &date);

private slots:
    void onSearchTextChanged(const QString &text);
    void onTagSearchClicked();
    void onDoubleClicked(const QModelIndex &index);
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();

private:
    InspirationModel *m_model;

    QStackedWidget *m_viewStack;
    QTableView *m_tableView;
    QListWidget *m_gridView;
    CalendarView *m_calendarView;
    QLineEdit *m_searchEdit;
    QStringList m_filterTags;
    bool m_filterMatchAll;
    void setupUI();
    void refreshGridView();
    void applyFilters();
    QWidget *m_leftBottomContainer;
    QCheckBox *m_dateFilterCheck;
    QSpinBox *m_yearSpin;
    QSpinBox *m_monthSpin;
    QComboBox *m_monthFilterCombo;
};

#endif // INSPIRATIONVIEW_H
