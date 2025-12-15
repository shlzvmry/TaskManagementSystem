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

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // 自定义操作
    bool addInspiration(const QString &content, const QString &tags = "");
    bool updateInspiration(int id, const QString &content, const QString &tags = "");
    bool deleteInspiration(int id);
    bool deleteInspirations(const QList<int> &ids);

    // 查询操作
    QList<QVariantMap> getAllInspirations() const;
    QList<QVariantMap> getInspirationsByDate(const QDate &date) const;
    QList<QVariantMap> getInspirationsByTag(const QString &tag) const;
    QList<QVariantMap> searchInspirations(const QString &keyword) const;

    // 统计操作
    int getInspirationCount() const;
    QList<QDate> getDatesWithInspirations() const;
    QStringList getAllTags() const;

    // 刷新数据
    void refresh();

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
