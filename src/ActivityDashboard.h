#ifndef ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
#define ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QBarSet>
#include <QChart>
#include <QRandomGenerator>
#include <QLabel>
#include <QTimer>

#include "DatabaseManager.h"
#include "Category.h"
#include "Mode.h"
#include "TimerState.h"
#include "FocusSession.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ActivityDashboard; }
QT_END_NAMESPACE

class ActivityDashboard : public QMainWindow {
 Q_OBJECT

 public:
    explicit ActivityDashboard(DatabaseManager *database_manager, QWidget *parent = nullptr);
    ~ActivityDashboard() override;

 private:
    Ui::ActivityDashboard *ui;
    DatabaseManager *database_manager;
    QSystemTrayIcon *tray_icon;
    QMenu *tray_menu;
    bool is_quitting = false;
    bool isDateToLastEdited = true;
    AppStats active_app;
    std::unordered_map<int, Category> categories;
    std::unordered_map<int, int> categories_indexes;
    int active_category_id = -1;
    Mode current_mode = Mode::Applications;
    const QString default_color = "#9E9E9E";
    QTimer *focus_timer;
    int timer_remaining_seconds = 25 * 60;
    int total_session_seconds;
    bool is_break_mode = false;
    TimerState timer_state = TimerState::Disabled;
    std::unordered_map<QString, bool> app_productivity_cache;
    std::vector<std::pair<QString, QString>> tray_messages;

    QString getDisplayTime(int64_t time);
    void createMenu();
    void refreshOverview(QDate &date_from, QDate &date_to);
    void updateActiveApp(int row);
    void updateDatesToWeekGap(QDate &date_from, QDate &date_to);
    QBarSet* setupWeekChart(std::vector<int64_t>& data, QChart* chart);
    void refreshDailyActivity(QDate &date_from, QDate &date_to);
    void refreshDetails(QDate &date_from, QDate &date_to);
    void saveAppSettings();
    void restoreHiddenApp();
    void updateHiddenAppsGroupBox();
    void updateCategories();
    void refreshCategorySettings();
    QString generateRandomColor();
    void inflictFadingEffectOnLabel(QLabel *label);
    void saveCategorySettings();
    void deleteCategory();
    void insertFocusSession();
    void updateTimerDisplay();
    void updateFocusTimerStyle();
    void refreshFocusSessionHistory(QDate &date_from, QDate &date_to);

 protected:
    void closeEvent(QCloseEvent *event) override;

 signals:
    void toggleFocusSessionFlag(bool is_focus_session);

 private slots:
    void refreshData();
    void iconActivated(QSystemTrayIcon::ActivationReason activation_reason);
    void tableItemLeftClicked(int row);
    void tableItemRightClicked(int row);
    void onFocusTimerTick();
    void btnFocusStartClicked();
    void btnFocusStopClicked();

 public slots:
    void currentAppChanged(const QString& exe_filename);
};

#endif //ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
