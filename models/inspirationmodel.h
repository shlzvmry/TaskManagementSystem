#ifndef INSPIRATIONMODEL_H
#define INSPIRATIONMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QList>
#include <QDateTime>

class InspirationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit InspirationModel(QObject *parent = nullptr);
    ~InspirationModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool addInspiration(const QString &content, const QString &tags = "");
    bool updateInspiration(int id, const QString &content, const QString &tags = "");
    bool deleteInspiration(int id);
    bool deleteInspirations(const QList<int> &ids);

    QList<QVariantMap> getAllInspirations() const;
    QList<QVariantMap> getInspirationsByDate(const QDate &date) const;
    QList<QVariantMap> getInspirationsByTag(const QString &tag) const;
    QList<QVariantMap> searchInspirations(const QString &keyword) const;

    int getInspirationCount() const;
    QList<QDate> getDatesWithInspirations() const;
    QList<QDate> getDatesWithInspirations(const QStringList &filterTags, bool matchAll) const;
    QStringList getAllTags() const;

    void refresh();

    bool restoreInspiration(int id);
    bool permanentDeleteInspiration(int id);
    QList<QVariantMap> getDeletedInspirations() const;
    bool emptyRecycleBin();

    bool renameTag(const QString &oldName, const QString &newName);
    bool removeTagFromAll(const QString &tagName);

signals:
    void inspirationAdded(int id);
    void inspirationUpdated(int id);
    void inspirationDeleted(int id);

private:
    struct InspirationItem {
        int id;
        QString content;
        QString tags;
        QDateTime createdAt;
        QDateTime updatedAt;

        QVariantMap toVariantMap() const {
            QVariantMap map;
            map["id"] = id;
            map["content"] = content;
            map["tags"] = tags;
            map["created_at"] = createdAt;
            map["updated_at"] = updatedAt;
            return map;
        }

        QString preview(int maxLength = 50) const {
            if (content.length() <= maxLength) {
                return content;
            }
            return content.left(maxLength) + "...";
        }

        QStringList tagList() const {
            if (tags.isEmpty()) return QStringList();
            return tags.split(",", Qt::SkipEmptyParts);
        }
    };

    QList<InspirationItem> inspirations;
    QSqlDatabase db;

    void loadInspirations();
    QDateTime getCurrentTimestamp() const;
};

#endif // INSPIRATIONMODEL_H
