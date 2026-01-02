#ifndef INSPIRATIONVIEW_H
#define INSPIRATIONVIEW_H

#include <QWidget>
#include <QStyledItemDelegate>
#include <QListWidget>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QDate>

class InspirationModel;
class QTableView;
class QLineEdit;
class QComboBox;

// 1. 定义网格视图的代理，负责画出“便利贴”效果
class InspirationGridDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit InspirationGridDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

// 日历视图类声明
class CalendarView;

class InspirationView : public QWidget
{
    Q_OBJECT

public:
    explicit InspirationView(QWidget *parent = nullptr);
    void setModel(InspirationModel *model);

public slots:
    void refresh();
    void setTaskModel(class TaskModel *model);

signals:
    void showRecycleBinRequested();
    void showTagManagerRequested();
    // 新增：日历相关的信号
    void showInspirationsRequested(const QDate &date);
    void showTasksRequested(const QDate &date);

private slots:
    void onSearchTextChanged(const QString &text);
    void onTagFilterChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onAddClicked();
    void onEditClicked(); // 新增槽函数
    void onDeleteClicked();

private:
    InspirationModel *m_model;

    // 视图组件
    QStackedWidget *m_viewStack;    // 新增：视图堆栈
    QTableView *m_tableView;        // 列表视图
    QListWidget *m_gridView;        // 新增：网格视图 (便利贴)
    CalendarView *m_calendarView;   // 新增：日历视图

    QLineEdit *m_searchEdit;
    QComboBox *m_tagCombo;

    void setupUI();
    void updateTagCombo();
    void refreshGridView();         // 新增：刷新网格数据
};

#endif // INSPIRATIONVIEW_H
