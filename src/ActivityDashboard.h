#ifndef ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
#define ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include "DatabaseManager.h"

//TODO: Добавить сверху панель с выбором мода: "Приложения, Категории, Фокус-сессии"

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
    AppStats active_app;
//    vector<Category> categories;

    QString getDisplayTime(int64_t time);
    void refreshOverview(QDate& date_from, QDate& date_to);
    void createMenu();
    void updateActiveApp(int row, int col);

 protected:
    void closeEvent(QCloseEvent *event) override;

 private slots:
    void refreshData();
    void iconActivated(QSystemTrayIcon::ActivationReason activation_reason);
    void tableItemDoubleClicked(int row, int col);
};

#endif //ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
