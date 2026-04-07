// You may need to build the project (run Qt uic code generator) to get "ui_ActivityDashboard.h" resolved

#include "ActivityDashboard.h"
#include "ui_ActivityDashboard.h"

ActivityDashboard::ActivityDashboard(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::ActivityDashboard) {
    ui->setupUi(this);
    ui->dateEditFrom->setDate(QDate::currentDate());
    ui->dateEditTo->setDate(QDate::currentDate());
    ui->tabWidget->setFocus();
}

ActivityDashboard::~ActivityDashboard() {
    delete ui;
}
void ActivityDashboard::refreshData() {
    qDebug() << "refreshed";
}
