#ifndef ACTIVITY_INSIGHT_DATABASE_MANAGER
#define ACTIVITY_INSIGHT_DATABASE_MANAGER

#include <QSqlDatabase>
#include <QDateTime>

#include "WindowData.h"
#include "AppStats.h"
#include "Category.h"
#include "FocusSession.h"

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
    std::vector<std::pair<int64_t, Category>> getUpdatedDailyCategoriesStats(const std::string& date_from = "today", const std::string& date_to = "today");
    std::unordered_map<int, Category> getCategories();
    std::vector<int64_t> getWeekUpdatedDailyStats(const QDate &date_from, const QDate &date_to);
    std::vector<int64_t> getWeekUpdatedAppStats(const QString &exe_filename, const QDate &date_from, const QDate &date_to);
    std::vector<int64_t> getWeekUpdatedCategoryStats(int category_id, const QDate &date_from, const QDate &date_to);
    void updateAppInfo(const AppStats& app_stats);
    std::vector<std::pair<QString, QString>> getHiddenApps();
    void restoreHiddenApp(const QString &exe_filename);
    void updateCategoryInfo(const Category& category);
    int insertNewCategory(const Category& category);
    void deleteCategory(int category_id);
    void insertFocusSession(int category_id, const QDateTime &session_datetime, int duration_secs);
    std::vector<std::pair<int, int64_t>> getFocusSessionsCategoriesStats(const QDate &date_from, const QDate &date_to);
    std::vector<FocusSession> getFocusSessions(const QDate &date_from, const QDate &date_to);
    bool isAppProductive(const QString &exe_filename);
};

#endif // ACTIVITY_INSIGHT_DATABASE_MANAGER