#include "comboboxdelegate.h"
#include "models/taskmodel.h"
#include <QComboBox>
#include <QApplication>

ComboBoxDelegate::ComboBoxDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option);

    if (index.column() != 3 && index.column() != 4) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    QComboBox *editor = new QComboBox(parent);
    editor->setFrame(false);

    if (index.column() == 3) {
        QMap<int, QString> options = TaskModel::getPriorityOptions();
        for (auto it = options.begin(); it != options.end(); ++it) {
            editor->addItem(it.value(), it.key());
        }
    } else if (index.column() == 4) {
        QMap<int, QString> options = TaskModel::getStatusOptions();
        for (auto it = options.begin(); it != options.end(); ++it) {
            editor->addItem(it.value(), it.key());
        }
    }

    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() != 3 && index.column() != 4) {
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    }

    QComboBox *comboBox = static_cast<QComboBox*>(editor);

    QString currentText = index.data(Qt::DisplayRole).toString();
    int cbIndex = comboBox->findText(currentText);

    if (cbIndex >= 0) {
        comboBox->setCurrentIndex(cbIndex);
    }

    comboBox->showPopup();
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    if (index.column() != 3 && index.column() != 4) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    int value = comboBox->currentData().toInt();

    if (index.column() == 3) {
        model->setData(index, value, TaskModel::PriorityRole);
    } else if (index.column() == 4) {
        model->setData(index, value, TaskModel::StatusRole);
    }
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}
