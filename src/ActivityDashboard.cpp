// You may need to build the project (run Qt uic code generator) to get "ui_ActivityDashboard.h" resolved

#include <QPieSeries>
#include <thread>
#include "ActivityDashboard.h"
#include "ui_ActivityDashboard.h"

ActivityDashboard::ActivityDashboard(DatabaseManager *database_manager, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::ActivityDashboard), database_manager(database_manager),
    tray_icon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);
    ui->overviewSplitter->setSizes({6000, 4000});
    ui->overviewSplitter->setStretchFactor(0, 6);
    ui->overviewSplitter->setStretchFactor(1, 4);
    ui->dateEditFrom->setDate(QDate::currentDate());
    ui->dateEditTo->setDate(QDate::currentDate());
    ui->tabWidget->setFocus();
    ui->tableOverviewList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableOverviewList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    createMenu();
    tray_icon->setContextMenu(tray_menu);

    QIcon app_icon = QIcon(":/assets/app_icon.png");
    tray_icon->setIcon(app_icon);
    this->setWindowIcon(app_icon);
    connect(tray_icon, &QSystemTrayIcon::activated, this, &ActivityDashboard::iconActivated);

    tray_icon->show();
    QMainWindow::showMaximized();
    refreshData();
}

ActivityDashboard::~ActivityDashboard() {
    delete ui;
}

void ActivityDashboard::createMenu() {
    auto show_hide_action = new QAction("Show");
    auto quit_action = new QAction("Quit");

    connect(show_hide_action, &QAction::triggered, this, [this](){
        this->setVisible(!this->isVisible());
    });
    connect(quit_action, &QAction::triggered, this, [this](){
        is_quitting = true;
        qApp->quit();
    });

    tray_menu = new QMenu(this);
    tray_menu->addAction(show_hide_action);
    tray_menu->addSeparator();
    tray_menu->addAction(quit_action);

    connect(tray_menu, &QMenu::aboutToShow, this, [this, show_hide_action](){
       if (this->isVisible()) {
           show_hide_action->setText("Hide");
       } else {
           show_hide_action->setText("Show");
       }
    });
}

void ActivityDashboard::refreshData() {
    qDebug() << "refreshed";

    QDate date_from = ui->dateEditFrom->date();
    QDate date_to = ui->dateEditTo->date();

    refreshOverview(date_from, date_to);
}

QString ActivityDashboard::getDisplayTime(int64_t time) {
    if (time < 60) {
        return {"less than 1 min"};
    }
    int64_t hours = time / 3600;
    int64_t minutes = time % 3600 / 60;
    if (hours == 0) {
        return QString::number(minutes) + QString(" min");
    } else {
        return QString::number(hours) + QString(" h ") + QString::number(minutes) + QString(" min");
    }
}

void ActivityDashboard::refreshOverview(QDate &date_from, QDate &date_to) {
    auto data = database_manager->getUpdatedDailyAppStats(date_from.toString("yyyy-MM-dd").toStdString(),
                                                          date_to.toString("yyyy-MM-dd").toStdString());
    int64_t time_sum = std::accumulate(data.begin(), data.end(), (int64_t) 0,
                                       [](auto& a, auto& b) {
        return a + b.total_time;
    });
    ui->tableOverviewList->setRowCount(data.size());

    QChart* chart = ui->chartPieOverview->chart();
    if (!chart) {
        chart = new QChart;
        ui->chartPieOverview->setChart(chart);
        ui->chartPieOverview->setRenderHint(QPainter::Antialiasing);
    }
    chart->removeAllSeries();

    QLabel* empty_data_label = ui->chartPieOverview->findChild<QLabel*>("emptyDataLabel");
    if (data.empty()) {
        if (!empty_data_label) {
            empty_data_label = new QLabel("No data for these dates", ui->chartPieOverview);
            empty_data_label->setObjectName("emptyDataLabel");
            empty_data_label->setAlignment(Qt::AlignCenter);
            empty_data_label->setStyleSheet("QLabel { color : orange; font-size : 20px; }");

            QVBoxLayout* overlay_layout = new QVBoxLayout(ui->chartPieOverview);
            overlay_layout->addWidget(empty_data_label);
        }
        empty_data_label->show();
        return;
    }
    if (empty_data_label) {
        empty_data_label->hide();
    }

    auto *pie_series = new QPieSeries;
    int64_t others_time = 0;
    for (int i = 0; i < data.size(); i++) {
        auto& app_stats = data[i];
        const QString q_display_name = QString::fromStdWString(app_stats.display_name);
        if (app_stats.total_time * 20 >= time_sum) {
            pie_series->append(q_display_name, app_stats.total_time);
        } else {
            others_time += app_stats.total_time;
        }
        QTableWidgetItem *app_item = new QTableWidgetItem(q_display_name);
        ui->tableOverviewList->setItem(i, 0, app_item);
        QTableWidgetItem *time_item = new QTableWidgetItem(getDisplayTime(app_stats.total_time));
        ui->tableOverviewList->setItem(i, 1, time_item);
    }
    if (others_time > 0) {
        pie_series->append("Others", others_time);
    }
    pie_series->setLabelsVisible();

    chart->addSeries(pie_series);
    chart->setTheme(QChart::ChartThemeDark);
//    chart->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false);
}

void ActivityDashboard::iconActivated(QSystemTrayIcon::ActivationReason activation_reason) {
    switch (activation_reason) {
        case QSystemTrayIcon::DoubleClick:
        case QSystemTrayIcon::Trigger:
        {
            this->setVisible(!this->isVisible());
            break;
        }
        default:
            ;
    }
}

void ActivityDashboard::closeEvent(QCloseEvent *event) {
    if (is_quitting) {
        event->accept();
    } else {
        this->hide();
        event->ignore();
    }
}
