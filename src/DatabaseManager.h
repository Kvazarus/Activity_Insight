#ifndef ACTIVITY_INSIGHT_DATABASE_MANAGER
#define ACTIVITY_INSIGHT_DATABASE_MANAGER

#include <QSqlDatabase>

#include "WindowData.h"
#include "AppStats.h"

class DatabaseManager : public QObject {
    Q_OBJECT
 private:
    QSqlDatabase db;
 public:
    DatabaseManager(QObject *parent = nullptr);
    void init();
    void insertActivityLog(const WindowData& window_data);
    void updateDailyStats();
    std::vector<AppStats> getUpdatedDailyAppStats(const std::string& date_from = "today", const std::string& date_to = "today");
};

#endif // ACTIVITY_INSIGHT_DATABASE_MANAGER