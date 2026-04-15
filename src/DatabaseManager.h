#ifndef ACTIVITY_INSIGHT_DATABASE_MANAGER
#define ACTIVITY_INSIGHT_DATABASE_MANAGER

#include <QSqlDatabase>

#include "WindowData.h"
#include "AppStats.h"
#include "Category.h"

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
    std::unordered_map<int, Category> getCategories();
    std::vector<int64_t> getWeekUpdatedDailyStats(QDate &date_from, QDate &date_to);
    std::vector<int64_t> getWeekUpdatedAppStats(const QString &exe_filename, QDate &date_from, QDate &date_to);
    void updateAppInfo(const AppStats& app_stats);
};

#endif // ACTIVITY_INSIGHT_DATABASE_MANAGER