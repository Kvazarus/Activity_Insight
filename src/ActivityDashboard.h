#ifndef ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
#define ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include "DatabaseManager.h"

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

    QString getDisplayTime(int64_t time);
    void refreshOverview(QDate& date_from, QDate& date_to);
    void createMenu();

 protected:
    void closeEvent(QCloseEvent *event) override;

 private slots:
    void refreshData();
    void iconActivated(QSystemTrayIcon::ActivationReason activation_reason);
};

#endif //ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
