#include "remindthread.h"
#include "database/database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QDebug>

RemindThread::RemindThread(QObject *parent) : QThread(parent), m_stop(false)
{
}

void RemindThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_stop = true;
    m_cond.wakeOne();
}

void RemindThread::run()
{
    QString connectionName = QString("remind_thread_%1").arg((quintptr)QThread::currentThreadId());

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(Database::instance().getDatabasePath());

        if (!db.open()) {
            qDebug() << "RemindThread: Failed to open database";
            return;
        }

        while (!m_stop) {
            QSqlQuery overdueQuery(db);
            overdueQuery.prepare("UPDATE tasks SET status = 3 WHERE status != 2 AND status != 3 AND deadline < ? AND is_deleted = 0");
            overdueQuery.addBindValue(QDateTime::currentDateTime());

            if (overdueQuery.exec() && overdueQuery.numRowsAffected() > 0) {
                emit taskOverdueUpdated();
            }

            QSqlQuery remindQuery(db);
            remindQuery.prepare("SELECT id, title FROM tasks WHERE is_reminded = 0 AND remind_time <= ? AND status != 2 AND is_deleted = 0");
            remindQuery.addBindValue(QDateTime::currentDateTime());

            if (remindQuery.exec()) {
                while (remindQuery.next()) {
                    int id = remindQuery.value("id").toInt();
                    QString title = remindQuery.value("title").toString();

                    emit remindTask(id, title);

                    QSqlQuery updateQuery(db);
                    updateQuery.prepare("UPDATE tasks SET is_reminded = 1 WHERE id = ?");
                    updateQuery.addBindValue(id);
                    updateQuery.exec();
                }
            }

            QMutexLocker locker(&m_mutex);
            if (!m_stop) {
                m_cond.wait(&m_mutex, 30000);
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}
