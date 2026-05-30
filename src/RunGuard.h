#ifndef ACTIVITY_INSIGHT_SRC_RUNGUARD_H_
#define ACTIVITY_INSIGHT_SRC_RUNGUARD_H_

#include <QString>
#include <QSharedMemory>
#include <QSystemSemaphore>

class RunGuard {
 public:
    explicit RunGuard(const QString &key);
    ~RunGuard();

    bool isAnotherRunning();
    bool tryToRun();
    void release();

 private:
    const QString key;
    const QString memLockKey;
    const QString sharedmemKey;

    QSharedMemory sharedMem;
    QSystemSemaphore memLock;

    Q_DISABLE_COPY(RunGuard)
};

#endif //ACTIVITY_INSIGHT_SRC_RUNGUARD_H_
