#include "inspirationmodel.h"
#include "database/database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>

InspirationModel::InspirationModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    db = Database::instance().getDatabase();
    refresh();
}

InspirationModel::~InspirationModel()
{
}

int InspirationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return inspirations.size();
}

int InspirationModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4; // 时间, 内容预览, 标签, 操作
}

QVariant InspirationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= inspirations.size())
        return QVariant();

    const InspirationItem &item = inspirations.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: return item.createdAt.toString("yyyy-MM-dd HH:mm:ss");
        case 1: return item.preview();
        case 2: return item.tags;
        case 3: return "操作";
        default: return QVariant();
        }

    case Qt::UserRole:
        return item.toVariantMap();

    case Qt::ToolTipRole:
        return QString("完整内容:\n%1\n\n标签: %2")
            .arg(item.content)
            .arg(item.tags);

    case Qt::TextAlignmentRole:
        if (index.column() == 1) return int(Qt::AlignLeft | Qt::AlignVCenter);
        return int(Qt::AlignCenter);

    default:
        return QVariant();
    }
}

QVariant InspirationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "记录时间";
        case 1: return "内容预览";
        case 2: return "标签";
        case 3: return "操作";
        default: return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags InspirationModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void InspirationModel::loadInspirations()
{
    beginResetModel();
    inspirations.clear();

    QSqlQuery query(db);
    query.prepare("SELECT * FROM inspirations ORDER BY created_at DESC");

    if (!query.exec()) {
        qDebug() << "加载灵感记录失败:" << query.lastError().text();
        endResetModel();
        return;
    }

    while (query.next()) {
        InspirationItem item;
        item.id = query.value("id").toInt();
        item.content = query.value("content").toString();
        item.tags = query.value("tags").toString();
        item.createdAt = query.value("created_at").toDateTime();
        item.updatedAt = query.value("updated_at").toDateTime();

        inspirations.append(item);
    }

    endResetModel();
}

bool InspirationModel::addInspiration(const QString &content, const QString &tags)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO inspirations (content, tags, created_at, updated_at) "
                  "VALUES (?, ?, ?, ?)");

    QDateTime now = getCurrentTimestamp();
    query.addBindValue(content);
    query.addBindValue(tags);
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        qDebug() << "添加灵感记录失败:" << query.lastError().text();
        return false;
    }

    int newId = query.lastInsertId().toInt();

    refresh();
    emit inspirationAdded(newId);

    return true;
}

bool InspirationModel::updateInspiration(int id, const QString &content, const QString &tags)
{
    QSqlQuery query(db);
    query.prepare("UPDATE inspirations SET content = ?, tags = ?, updated_at = ? WHERE id = ?");

    query.addBindValue(content);
    query.addBindValue(tags);
    query.addBindValue(getCurrentTimestamp());
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "更新灵感记录失败:" << query.lastError().text();
        return false;
    }

    refresh();
    emit inspirationUpdated(id);

    return true;
}


bool InspirationModel::deleteInspiration(int id)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM inspirations WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "删除灵感记录失败:" << query.lastError().text();
        return false;
    }

    refresh();
    emit inspirationDeleted(id);

    return true;
}

bool InspirationModel::deleteInspirations(const QList<int> &ids)
{
    if (ids.isEmpty()) return true;

    db.transaction();

    QSqlQuery query(db);
    query.prepare("DELETE FROM inspirations WHERE id = ?");

    for (int id : ids) {
        query.addBindValue(id);
        if (!query.exec()) {
            qDebug() << "批量删除灵感记录失败:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    db.commit();

    refresh();

    return true;
}

QList<QVariantMap> InspirationModel::getAllInspirations() const
{
    QList<QVariantMap> result;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM inspirations ORDER BY created_at DESC");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["id"] = query.value("id").toInt();
            item["content"] = query.value("content").toString();
            item["tags"] = query.value("tags").toString();
            item["created_at"] = query.value("created_at").toDateTime();
            item["updated_at"] = query.value("updated_at").toDateTime();
            result.append(item);
        }
    }

    return result;
}

QList<QVariantMap> InspirationModel::getInspirationsByDate(const QDate &date) const
{
    QList<QVariantMap> result;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM inspirations "
                  "WHERE DATE(created_at) = ? "
                  "ORDER BY created_at DESC");
    query.addBindValue(date.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["id"] = query.value("id").toInt();
            item["content"] = query.value("content").toString();
            item["tags"] = query.value("tags").toString();
            item["created_at"] = query.value("created_at").toDateTime();
            item["updated_at"] = query.value("updated_at").toDateTime();
            result.append(item);
        }
    }

    return result;
}

QList<QVariantMap> InspirationModel::getInspirationsByTag(const QString &tag) const
{
    QList<QVariantMap> result;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM inspirations "
                  "WHERE tags LIKE ? "
                  "ORDER BY created_at DESC");
    query.addBindValue("%" + tag + "%");

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["id"] = query.value("id").toInt();
            item["content"] = query.value("content").toString();
            item["tags"] = query.value("tags").toString();
            item["created_at"] = query.value("created_at").toDateTime();
            item["updated_at"] = query.value("updated_at").toDateTime();
            result.append(item);
        }
    }

    return result;
}

QList<QVariantMap> InspirationModel::searchInspirations(const QString &keyword) const
{
    QList<QVariantMap> result;

    if (keyword.isEmpty()) {
        return getAllInspirations();
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM inspirations "
                  "WHERE content LIKE ? OR tags LIKE ? "
                  "ORDER BY created_at DESC");
    QString searchPattern = "%" + keyword + "%";
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap item;
            item["id"] = query.value("id").toInt();
            item["content"] = query.value("content").toString();
            item["tags"] = query.value("tags").toString();
            item["created_at"] = query.value("created_at").toDateTime();
            item["updated_at"] = query.value("updated_at").toDateTime();
            result.append(item);
        }
    }

    return result;
}

int InspirationModel::getInspirationCount() const
{
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM inspirations");

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

QList<QDate> InspirationModel::getDatesWithInspirations() const
{
    QList<QDate> dates;

    QSqlQuery query(db);
    query.prepare("SELECT DISTINCT DATE(created_at) as date FROM inspirations ORDER BY date DESC");

    if (query.exec()) {
        while (query.next()) {
            dates.append(query.value("date").toDate());
        }
    }

    return dates;
}

QStringList InspirationModel::getAllTags() const
{
    QStringList allTags;
    QSet<QString> uniqueTags;

    QSqlQuery query(db);
    query.prepare("SELECT tags FROM inspirations WHERE tags IS NOT NULL AND tags != ''");

    if (query.exec()) {
        while (query.next()) {
            QString tags = query.value("tags").toString();
            QStringList tagList = tags.split(",", Qt::SkipEmptyParts);
            for (const QString &tag : tagList) {
                uniqueTags.insert(tag.trimmed());
            }
        }
    }

    allTags = uniqueTags.values();
    allTags.sort();

    return allTags;
}

void InspirationModel::refresh()
{
    loadInspirations();
}

QDateTime InspirationModel::getCurrentTimestamp() const
{
    return QDateTime::currentDateTime();
}
