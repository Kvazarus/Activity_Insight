#ifndef ACTIVITY_INSIGHT_DATABASE_MANAGER
#define ACTIVITY_INSIGHT_DATABASE_MANAGER

#include <QSqlDatabase>

#include "WindowData.h"
#include "AppStats.h"

class DatabaseManager {
 private:
    QSqlDatabase db;
 public:
    DatabaseManager();
    void Init();
    void InsertActivityLog(WindowData& window_data);
    void UpdateDailyStats();
    std::vector<AppStats> GetUpdatedDailyAppStats(const std::string& date_from = "today", const std::string& date_to = "today");
};

#endif // ACTIVITY_INSIGHT_DATABASE_MANAGER