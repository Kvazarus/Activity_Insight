// You may need to build the project (run Qt uic code generator) to get "ui_ActivityDashboard.h" resolved

#include <QPieSeries>
#include <thread>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "ActivityDashboard.h"
#include "ui_ActivityDashboard.h"

// TODO: Добавить в настройках приложений опцию: вернуть скрытые приложения
// TODO: Добавить QColorDialog в настройках категорий
// TODO: Добавить в Overview категорий разные метрики

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
    ui->tableOverviewList->hideColumn(2); // exe_filename or id (category)
    ui->tableOverviewList->hideColumn(3); // is_hidden
    ui->tableOverviewList->hideColumn(4); // category_id
    ui->tableOverviewList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->lblSavedStatus->hide();
    ui->comboMode->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);

    connect(ui->tableOverviewList, &QTableWidget::cellClicked, this, &ActivityDashboard::tableItemLeftClicked);
    connect(ui->tableOverviewList, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QTableWidgetItem *item = ui->tableOverviewList->itemAt(pos);
        if (item) {
            tableItemRightClicked(item->row());
        }
    });
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
    connect(ui->btnSaveSettings, &QPushButton::clicked, this, &ActivityDashboard::saveAppSettings);
    connect(ui->btnRestoreHidden, &QPushButton::clicked, this, &ActivityDashboard::restoreHiddenApp);
    connect(ui->comboMode, &QComboBox::currentIndexChanged, this, [this](int index) {
        switch (index) {
            case 0:
                current_mode = Mode::Applications;
                break;
            case 1:
                current_mode = Mode::Categories;
                break;
            case 2:
                current_mode = Mode::FocusSessions;
                break;
            default:
                qDebug() << "Wrong index in comboMode";
        }
        refreshData();
    });

    updateHiddenAppsGroupBox();

    createMenu();
    tray_icon->setContextMenu(tray_menu);
    QIcon app_icon = QIcon(":/assets/app_icon.png");
    tray_icon->setIcon(app_icon);
    this->setWindowIcon(app_icon);
    connect(tray_icon, &QSystemTrayIcon::activated, this, &ActivityDashboard::iconActivated);
    tray_icon->show();

    categories = database_manager->getCategories();
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

    if (ui->tabWidget->currentIndex() == 0) {
        refreshOverview(date_from, date_to);
    } else if (ui->tabWidget->currentIndex() == 1 || ui->tabWidget->currentIndex() == 2) {
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
    } else if (ui->tabWidget->currentIndex() == 3) {
        if (current_mode == Mode::Applications) {
            ui->stackedWidgetSettings->setCurrentIndex(0);
        } else if (current_mode == Mode::Categories) {
            ui->stackedWidgetSettings->setCurrentIndex(1);
        }
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
    bool is_app_mode = current_mode == Mode::Applications;
    int64_t time_sum;
    std::vector<AppStats> data_apps;
    std::vector<std::pair<int64_t, Category>> data_categories;
    if (is_app_mode) {
        data_apps = database_manager->getUpdatedDailyAppStats(date_from.toString("yyyy-MM-dd").toStdString(),
                                                              date_to.toString("yyyy-MM-dd").toStdString());
        time_sum = std::accumulate(data_apps.begin(), data_apps.end(), (int64_t) 0,
                                           [](auto& a, auto& b) {
            return a + b.total_time;
        });
    } else {
        data_categories = database_manager->getUpdatedDailyCategoriesStats(date_from.toString("yyyy-MM-dd").toStdString(),
                                                              date_to.toString("yyyy-MM-dd").toStdString());
        time_sum = std::accumulate(data_categories.begin(), data_categories.end(), (int64_t) 0,
                                   [](auto& a, auto& b) {
                                       return a + b.first;
        });
    }

    ui->lblTotalTime->setText("Total time: " + getDisplayTime(time_sum));
    if (is_app_mode) {
        ui->tableOverviewList->setRowCount(data_apps.size());
    } else {
        ui->tableOverviewList->setRowCount(data_categories.size());
    }

    QChart* chart = ui->chartPieOverview->chart();
    if (!chart) {
        chart = new QChart;
        ui->chartPieOverview->setChart(chart);
        ui->chartPieOverview->setRenderHint(QPainter::Antialiasing);
    }
    chart->removeAllSeries();

    auto empty_data_label = ui->chartPieOverview->findChild<QLabel*>("emptyDataLabelOverview");
    if ((is_app_mode && data_apps.empty()) || (!is_app_mode && data_categories.empty())) {
        if (!empty_data_label) {
            empty_data_label = new QLabel("No data for these dates", ui->chartPieOverview);
            empty_data_label->setObjectName("emptyDataLabelOverview");
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
    for (int i = 0; i < (is_app_mode ? data_apps.size() : data_categories.size()); i++) {
        QString name = is_app_mode ? data_apps[i].display_name : data_categories[i].second.name;
        int64_t total_time = is_app_mode ? data_apps[i].total_time : data_categories[i].first;
        QPieSlice *slice = new QPieSlice(name, total_time);
        if (!is_app_mode) slice->setColor(data_categories[i].second.color);
        if (total_time * 20 >= time_sum) {
            pie_series->append(slice);
            connect(slice, &QPieSlice::hovered, this, [slice](bool flag){
                slice->setExploded(flag);
            });
            connect(slice, &QPieSlice::pressed, this, [this, i](){
                if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
                    tableItemLeftClicked(i);
                } else if (QGuiApplication::mouseButtons() & Qt::RightButton) {
                    tableItemRightClicked(i);
                }
            });
        } else {
            others_time += total_time;
        }
        QTableWidgetItem *app_item = new QTableWidgetItem(name);
        ui->tableOverviewList->setItem(i, 0, app_item);
        QTableWidgetItem *time_item = new QTableWidgetItem(getDisplayTime(total_time));
        ui->tableOverviewList->setItem(i, 1, time_item);
        QTableWidgetItem *exe_filename_or_id_item = new QTableWidgetItem(is_app_mode ? data_apps[i].exe_filename : QString::number(data_categories[i].second.id));
        ui->tableOverviewList->setItem(i, 2, exe_filename_or_id_item);
        QTableWidgetItem *is_hidden_item = new QTableWidgetItem(is_app_mode ? QString::number(data_apps[i].is_hidden) : "");
        ui->tableOverviewList->setItem(i, 3, is_hidden_item);
        QTableWidgetItem *category_id_item = new QTableWidgetItem(is_app_mode ? QString::number(data_apps[i].category_id) : "");
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

void ActivityDashboard::tableItemLeftClicked(int row) {
    if (current_mode == Mode::Applications) {
        updateActiveApp(row);
    } else {
        active_category_id = ui->tableOverviewList->item(row, 2)->text().toInt();
    }
    ui->tabWidget->setCurrentIndex(2);
}

void ActivityDashboard::tableItemRightClicked(int row) {
    if (current_mode == Mode::Applications) {
        updateActiveApp(row);

        ui->editSettingsPath->setText(active_app.exe_filename);
        ui->editSettingsName->setText(active_app.display_name);

        ui->comboSettingsCategory->clear();
        int ind = 0;
        for(auto &[id, cat] : categories) {
            ui->comboSettingsCategory->addItem(cat.name, id);
            if (id == active_app.category_id) {
                ui->comboSettingsCategory->setCurrentIndex(ind);
            }
            ind++;
        }

        ui->checkSettingsHidden->setCheckState(active_app.is_hidden ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        updateHiddenAppsGroupBox();
    } else {
        active_category_id = ui->tableOverviewList->item(row, 2)->text().toInt();
        // do something here later
    }
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
    // По сути категории и приложения по суммарному времени совпадают, поэтому для категорий тут можно ничего не менять
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
    if (!chart) {
        chart = new QChart;
        ui->chartBarDailyTotal->setChart(chart);
        ui->chartBarDailyTotal->setRenderHint(QPainter::Antialiasing);
    }

    auto empty_data_label = ui->chartBarAppDetails->findChild<QLabel*>("emptyDataLabelDetails");

    if ((current_mode == Mode::Applications && active_app.exe_filename.isEmpty()) ||
    (current_mode == Mode::Categories && !active_category_id)) {
        if (!empty_data_label) {
            empty_data_label = new QLabel("Click an application from the Overview", ui->chartBarAppDetails);
            empty_data_label->setObjectName("emptyDataLabelDetails");
            empty_data_label->setAlignment(Qt::AlignCenter);
            empty_data_label->setStyleSheet("QLabel { color : orange; font-size : 20px; }");

            QVBoxLayout* overlay_layout = new QVBoxLayout(ui->chartBarAppDetails);
            overlay_layout->addWidget(empty_data_label);
        }
        if (current_mode == Mode::Applications) {
            empty_data_label->setText("Click an application from the Overview");
            ui->lblDetailsAppName->setText("App Details");
        } else {
            empty_data_label->setText("Click a category from the Overview");
            ui->lblDetailsAppName->setText("Category Details");
        }
        empty_data_label->show();

        chart->hide();
        chart->removeAllSeries();
        chart->setTheme(QChart::ChartThemeDark);
        chart->setBackgroundVisible(false);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        return;
    }
    if (empty_data_label) {
        empty_data_label->hide();
    }
    chart->show();


    std::vector<int64_t> data;
    if (current_mode == Mode::Applications) {
        ui->lblDetailsAppName->setText(active_app.display_name);
        data = database_manager->getWeekUpdatedAppStats(active_app.exe_filename, date_from, date_to);
    } else {
        ui->lblDetailsAppName->setText(categories[active_category_id].name);
        data = database_manager->getWeekUpdatedCategoryStats(active_category_id, date_from, date_to);
    }

    auto bar_set = setupWeekChart(data, chart);
    if (current_mode == Mode::Applications) {
        bar_set->setColor("orange");
    } else {
        bar_set->setColor(categories[active_category_id].color);
    }
}

void ActivityDashboard::updateHiddenAppsGroupBox() {
    auto hidden_apps = database_manager->getHiddenApps();
    if (hidden_apps.empty()) {
        ui->groupBoxHiddenApps->hide();
        return;
    }
    ui->groupBoxHiddenApps->show();

    ui->listHiddenApps->clear();
    for (auto &[exe_filename, display_name] : hidden_apps) {
        auto new_item = new QListWidgetItem();
        new_item->setText(display_name);
        new_item->setData(Qt::UserRole, QVariant(exe_filename));
        ui->listHiddenApps->addItem(new_item);
    }
}

void ActivityDashboard::saveAppSettings() {
    QString exe_filename = ui->editSettingsPath->text();
    QString display_name = ui->editSettingsName->text();
    int category_id = ui->comboSettingsCategory->currentData().toInt();
    bool is_hidden = ui->checkSettingsHidden->isChecked();
    if (exe_filename.isEmpty() || display_name.isEmpty()) {
        ui->lblSavedStatus->setText("Incorrect input");
        ui->lblSavedStatus->setStyleSheet("QLabel { color : red; }");
    } else {
        database_manager->updateAppInfo({exe_filename, display_name, is_hidden, category_id,0});
        ui->lblSavedStatus->setText("Saved!");
        ui->lblSavedStatus->setStyleSheet("QLabel { color : #4CAF50; }");
        if (active_app.is_hidden != is_hidden) {
            updateHiddenAppsGroupBox();
        }
        active_app.display_name = display_name;
        active_app.category_id = category_id;
        active_app.is_hidden = is_hidden;
    }
    ui->lblSavedStatus->show();
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    ui->lblSavedStatus->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(2000);
    a->setStartValue(1);
    a->setEndValue(0);
    a->setEasingCurve(QEasingCurve::InQuad);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(a, &QPropertyAnimation::finished, ui->lblSavedStatus, &QLabel::hide);
}

void ActivityDashboard::restoreHiddenApp() {
    QListWidgetItem *current_item = ui->listHiddenApps->currentItem();
    if (current_item) {
        QString exe_filename = current_item->data(Qt::UserRole).toString();
        database_manager->restoreHiddenApp(exe_filename);
        updateHiddenAppsGroupBox();
        if (active_app.exe_filename == exe_filename) {
            active_app.is_hidden = false;
            ui->checkSettingsHidden->setCheckState(Qt::CheckState::Unchecked);
        }
    }
}
