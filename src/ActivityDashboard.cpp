// You may need to build the project (run Qt uic code generator) to get "ui_ActivityDashboard.h" resolved

#include <QPieSeries>
#include <thread>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
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
    ui->dateEditFrom->setKeyboardTracking(false);
    ui->dateEditTo->setKeyboardTracking(false);
    ui->tabWidget->setFocus();
    ui->tableOverviewList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableOverviewList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableOverviewList->setColumnCount(5);
    ui->tableOverviewList->hideColumn(2); // exe_filename
    ui->tableOverviewList->hideColumn(3); // is_hidden
    ui->tableOverviewList->hideColumn(4); // category_id

    connect(ui->tableOverviewList, &QTableWidget::cellClicked, this, &ActivityDashboard::tableItemClicked);
    connect(ui->tableOverviewList, &QTableWidget::cellDoubleClicked, this, &ActivityDashboard::tableItemDoubleClicked);
    connect(ui->dateEditFrom, &QDateEdit::dateChanged, this, [this](){
       isDateToLastEdited = false;
       refreshData();
    });
    connect(ui->dateEditTo, &QDateEdit::dateChanged, this, [this](){
        isDateToLastEdited = true;
        refreshData();
    });
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &ActivityDashboard::refreshData);
    connect(ui->btnDailyPrev, &QToolButton::clicked, this, [this](){
        // Это автоматически подтянет dateEditTo и обновит страницу благодаря сигналам
        ui->dateEditFrom->setDate(ui->dateEditFrom->date().addDays(-7));
    });
    connect(ui->btnDailyNext, &QToolButton::clicked, this, [this](){
        ui->dateEditFrom->setDate(ui->dateEditFrom->date().addDays(7));
    });
    connect(ui->btnPrevWeek, &QToolButton::clicked, this, [this](){
        ui->dateEditFrom->setDate(ui->dateEditFrom->date().addDays(-7));
    });
    connect(ui->btnNextWeek, &QToolButton::clicked, this, [this](){
        ui->dateEditFrom->setDate(ui->dateEditFrom->date().addDays(7));
    });

    createMenu();
    tray_icon->setContextMenu(tray_menu);
    QIcon app_icon = QIcon(":/assets/app_icon.png");
    tray_icon->setIcon(app_icon);
    this->setWindowIcon(app_icon);
    connect(tray_icon, &QSystemTrayIcon::activated, this, &ActivityDashboard::iconActivated);
    tray_icon->show();

    categories = database_manager->getCategories();
    QMainWindow::showMaximized();
    ui->tabWidget->setCurrentIndex(0);
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
        if (this->isVisible()) this->activateWindow();
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

    if (ui->tabWidget->currentIndex() == 1 || ui->tabWidget->currentIndex() == 2) {
        updateDatesToWeekGap(date_from, date_to);

        ui->dateEditFrom->blockSignals(true);
        ui->dateEditTo->blockSignals(true);
        ui->dateEditFrom->setDate(date_from);
        ui->dateEditTo->setDate(date_to);
        ui->dateEditFrom->blockSignals(false);
        ui->dateEditTo->blockSignals(false);

        if (ui->tabWidget->currentIndex() == 1) {
            refreshDailyActivity(date_from, date_to);
        } else {
            refreshDetails(date_from, date_to);
        }
    } else {
        refreshOverview(date_from, date_to);
    }
}

QString ActivityDashboard::getDisplayTime(int64_t time) {
    if (time == 0) return {"0 min"};
    if (time < 60) {
        return {"less than 1 min"};
    }
    int64_t days = time / 3600 / 24;
    int64_t hours = time / 3600 % 24;
    int64_t minutes = time % 3600 / 60;
    if (hours == 0 && days == 0) {
        return QString::number(minutes) + QString(" min");
    } else if (days == 0) {
        return QString::number(hours) + QString(" h ") + QString::number(minutes) + QString(" min");
    } else {
        return QString::number(days) + QString(" days ") + QString::number(hours) + QString(" h ") +
            QString::number(minutes) + QString(" min");
    }
}

void ActivityDashboard::refreshOverview(QDate &date_from, QDate &date_to) {
    auto data = database_manager->getUpdatedDailyAppStats(date_from.toString("yyyy-MM-dd").toStdString(),
                                                          date_to.toString("yyyy-MM-dd").toStdString());
    int64_t time_sum = std::accumulate(data.begin(), data.end(), (int64_t) 0,
                                       [](auto& a, auto& b) {
        return a + b.total_time;
    });
    ui->lblTotalTime->setText("Total time: " + getDisplayTime(time_sum));
    ui->tableOverviewList->setRowCount(data.size());

    QChart* chart = ui->chartPieOverview->chart();
    if (!chart) {
        chart = new QChart;
        ui->chartPieOverview->setChart(chart);
        ui->chartPieOverview->setRenderHint(QPainter::Antialiasing);
    }
    chart->removeAllSeries();

    auto empty_data_label = ui->chartPieOverview->findChild<QLabel*>("emptyDataLabelOverviewApps");
    if (data.empty()) {
        if (!empty_data_label) {
            empty_data_label = new QLabel("No data for these dates", ui->chartPieOverview);
            empty_data_label->setObjectName("emptyDataLabelOverviewApps");
            empty_data_label->setAlignment(Qt::AlignCenter);
            empty_data_label->setStyleSheet("QLabel { color : orange; font-size : 20px; }");

            QVBoxLayout* overlay_layout = new QVBoxLayout(ui->chartPieOverview);
            overlay_layout->addWidget(empty_data_label);
        }
        empty_data_label->show();
        chart->setTheme(QChart::ChartThemeDark);
        chart->setBackgroundVisible(false);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        return;
    }
    if (empty_data_label) {
        empty_data_label->hide();
    }

    auto *pie_series = new QPieSeries;
    int64_t others_time = 0;
    for (int i = 0; i < data.size(); i++) {
        auto& app_stats = data[i];
        QPieSlice *slice = new QPieSlice(app_stats.display_name, app_stats.total_time);
        if (app_stats.total_time * 20 >= time_sum) {
            pie_series->append(slice);
            connect(slice, &QPieSlice::hovered, this, [slice](bool flag){
                slice->setExploded(flag);
            });
            connect(slice, &QPieSlice::clicked, this, [this, i](){
                tableItemClicked(i);
            });
            connect(slice, &QPieSlice::doubleClicked, this, [this, i](){
                tableItemDoubleClicked(i);
            });
        } else {
            others_time += app_stats.total_time;
        }
        QTableWidgetItem *app_item = new QTableWidgetItem(app_stats.display_name);
        ui->tableOverviewList->setItem(i, 0, app_item);
        QTableWidgetItem *time_item = new QTableWidgetItem(getDisplayTime(app_stats.total_time));
        ui->tableOverviewList->setItem(i, 1, time_item);
        QTableWidgetItem *exe_filename_item = new QTableWidgetItem(app_stats.exe_filename);
        ui->tableOverviewList->setItem(i, 2, exe_filename_item);
        QTableWidgetItem *is_hidden_item = new QTableWidgetItem(QString::number(app_stats.is_hidden));
        ui->tableOverviewList->setItem(i, 3, is_hidden_item);
        QTableWidgetItem *category_id_item = new QTableWidgetItem(QString::number(app_stats.category_id));
        ui->tableOverviewList->setItem(i, 4, category_id_item);
    }
    if (others_time > 0) {
        pie_series->append("Others", others_time);
    }
    pie_series->setLabelsVisible();

    chart->addSeries(pie_series);
    chart->setTheme(QChart::ChartThemeDark);
    chart->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false);
}

void ActivityDashboard::iconActivated(QSystemTrayIcon::ActivationReason activation_reason) {
    switch (activation_reason) {
        case QSystemTrayIcon::Trigger:
        {
            this->setVisible(!this->isVisible());
            if (this->isVisible()) this->activateWindow();
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

void ActivityDashboard::updateActiveApp(int row) {
    auto table = ui->tableOverviewList;
    active_app.display_name = table->item(row, 0)->text();
    active_app.total_time = table->item(row, 1)->text().toLongLong();
    active_app.exe_filename = table->item(row, 2)->text();
    active_app.is_hidden = table->item(row, 3)->text().toInt();
    active_app.category_id = table->item(row, 4)->text().toInt();
}

void ActivityDashboard::tableItemClicked(int row) {
    updateActiveApp(row);
    ui->tabWidget->setCurrentIndex(2);
}

void ActivityDashboard::tableItemDoubleClicked(int row) {
    updateActiveApp(row);
    ui->editSettingsPath->setText(active_app.exe_filename);
    ui->editSettingsName->setText(active_app.display_name);

    ui->comboSettingsCategory->clear();
    for(auto &[ind, cat] : categories) {
        ui->comboSettingsCategory->addItem(cat.name, ind);
    }

    ui->checkSettingsHidden->setCheckState(active_app.is_hidden ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->tabWidget->setCurrentIndex(3);
}

void ActivityDashboard::updateDatesToWeekGap(QDate &date_from, QDate &date_to) {
    QDate date = isDateToLastEdited ? date_to : date_from;
    date_from = date.addDays(-(date.dayOfWeek() - 1));
    date_to = date_from.addDays(6);
}

QBarSet* ActivityDashboard::setupWeekChart(std::vector<int64_t> &data, QChart* chart) {
    if (!chart) {
        chart = new QChart;
        ui->chartBarDailyTotal->setChart(chart);
        ui->chartBarDailyTotal->setRenderHint(QPainter::Antialiasing);
    }
    chart->removeAllSeries();

    for (auto axis : chart->axes()) {
        chart->removeAxis(axis);
    }

    auto series = new QBarSeries();
    auto bar_set = new QBarSet("Activity");
    QStringList days{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    QStringList custom_labels;
    for (int i = 0; i < 7; i++) {
        *bar_set << (qreal) data[i] / 3600.0;
        custom_labels.append(days[i] + " (" + getDisplayTime(data[i]) + ")");
    }
    series->append(bar_set);
    series->setLabelsVisible(false);

    chart->addSeries(series);

    auto axisX = new QBarCategoryAxis();
    axisX->setLabelsVisible(true);
    axisX->append(custom_labels);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto axisY = new QValueAxis();
    axisY->setLabelsVisible(true);
    axisY->setMin(0);
    qreal max_time_hrs = (qreal) *std::max_element(data.begin(), data.end()) / 3600.0;
    axisY->setMax(max_time_hrs > 0 ? max_time_hrs * 1.15 : 10);
    axisY->setLabelFormat("%.0f");
    axisY->setTitleText("Hours");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setTheme(QChart::ChartThemeDark);
    chart->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false);

    auto font = QFont();
    font.setPixelSize(13);
    font.setWeight(QFont::Weight::Bold);
    axisY->setTitleFont(font);

    return bar_set;
}

void ActivityDashboard::refreshDailyActivity(QDate &date_from, QDate &date_to) {
    auto data = database_manager->getWeekUpdatedDailyStats(date_from, date_to);

    QChart* chart = ui->chartBarDailyTotal->chart();
    auto bar_set = setupWeekChart(data, chart);

    connect(bar_set, &QBarSet::clicked, this, [this](int index) {
        ui->tabWidget->setCurrentIndex(0);
        QDate date = ui->dateEditFrom->date().addDays(index);
        ui->dateEditFrom->setDate(date);
        ui->dateEditTo->setDate(date);
    });
}

void ActivityDashboard::refreshDetails(QDate &date_from, QDate &date_to) {
    QChart* chart = ui->chartBarAppDetails->chart();
    auto empty_data_label = ui->chartBarAppDetails->findChild<QLabel*>("emptyDataLabelDetailsApps");

    if (active_app.exe_filename.isEmpty()) {
        if (!empty_data_label) {
            empty_data_label = new QLabel("Click an application from the Overview", ui->chartBarAppDetails);
            empty_data_label->setObjectName("emptyDataLabelDetailsApps");
            empty_data_label->setAlignment(Qt::AlignCenter);
            empty_data_label->setStyleSheet("QLabel { color : orange; font-size : 20px; }");

            QVBoxLayout* overlay_layout = new QVBoxLayout(ui->chartBarAppDetails);
            overlay_layout->addWidget(empty_data_label);
        }
        empty_data_label->show();

        chart->removeAllSeries();
        chart->setTheme(QChart::ChartThemeDark);
        chart->setBackgroundVisible(false);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        return;
    }
    if (empty_data_label) {
        empty_data_label->hide();
    }

    ui->lblDetailsAppName->setText(active_app.display_name);
    auto data = database_manager->getWeekUpdatedAppStats(active_app.exe_filename, date_from, date_to);

    auto bar_set = setupWeekChart(data, chart);
    bar_set->setColor("orange");
}
