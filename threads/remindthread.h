#ifndef REMINDTHREAD_H
#define REMINDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class RemindThread : public QThread
{
    Q_OBJECT
public:
    explicit RemindThread(QObject *parent = nullptr);
    void stop();

signals:
    void taskOverdueUpdated();
    void remindTask(int taskId, QString title);

protected:
    void run() override;

private:
    bool m_stop;
    QMutex m_mutex;
    QWaitCondition m_cond;
};

#endif // REMINDTHREAD_H
