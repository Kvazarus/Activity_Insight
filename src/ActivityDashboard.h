#ifndef ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
#define ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QBarSet>
#include <QChart>

#include "DatabaseManager.h"
#include "Category.h"

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

 protected:
    void closeEvent(QCloseEvent *event) override;

 private slots:
    void refreshData();
    void iconActivated(QSystemTrayIcon::ActivationReason activation_reason);
    void tableItemLeftClicked(int row);
    void tableItemRightClicked(int row);
};

#endif //ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
