#ifndef ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
#define ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_

#include <QMainWindow>
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

    QString getDisplayTime(int64_t time);
    void refreshOverview(QDate& date_from, QDate& date_to);


 private slots:
    void refreshData();
};

#endif //ACTIVITY_INSIGHT_SRC_ACTIVITYDASHBOARD_H_
