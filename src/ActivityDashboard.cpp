// You may need to build the project (run Qt uic code generator) to get "ui_ActivityDashboard.h" resolved

#include <QPieSeries>
#include <thread>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QColorDialog>
#include <QMessageBox>
#include <iostream>

#include "ActivityDashboard.h"
#include "ui_ActivityDashboard.h"

ActivityDashboard::ActivityDashboard(DatabaseManager *database_manager, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::ActivityDashboard), database_manager(database_manager),
    tray_icon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);
    ui->overviewSplitter->setSizes({6000, 4000});
    ui->overviewSplitter->setStretchFactor(0, 6);
    ui->overviewSplitter->setStretchFactor(1, 4);
    ui->focusStatsSplitter->setSizes({6000, 4000});
    ui->focusStatsSplitter->setStretchFactor(0, 6);
    ui->focusStatsSplitter->setStretchFactor(1, 4);
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
    ui->tableFocusHistory->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableFocusHistory->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableFocusHistory->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->mainStackedWidget->setCurrentIndex(0);
    ui->focusTabWidget->setCurrentIndex(0);
    updateFocusTimerStyle();

    QSizePolicy sp = ui->lblSavedStatus->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    ui->lblSavedStatus->setSizePolicy(sp);

    sp = ui->lblCategorySavedStatus->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    ui->lblCategorySavedStatus->setSizePolicy(sp);

    sp = ui->lblCategoryDeletedStatus->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    ui->lblCategoryDeletedStatus->setSizePolicy(sp);

    ui->lblSavedStatus->hide();
    ui->lblCategorySavedStatus->hide();
    ui->lblCategoryDeletedStatus->hide();

    ui->comboMode->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);
    ui->lblColorPreview->setProperty("category_color", default_color);

    ui->productivityNotificationsCheckBox->setCheckState(Qt::CheckState::Checked);

    tray_messages = {
        {"Focus check", "You're still in a focus session. Time to get back?"},
        {"Wandering off?", "This app isn't on your productive list. Stay focused!"},
        {"Gentle reminder", "Your focus timer is still ticking in the background."},
        {"Off track?", "Looks like a distraction. Let's get back to work!"},
        {"Caught in 4K \U0001F4F8", "Does this look like work to you?"},
        {"Productivity Police \U0001F6A8", "Step away from the distraction and nobody gets hurt!"},
        {"Oops, wrong app!", "Your focus timer is secretly judging you right now."},
        {"Is this a break?", "Because the timer definitely didn't pause itself..."},
        {"Hello there! \U0001F44B", "Just your friendly neighborhood timer reminding you to work."}
    };

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
                ui->mainStackedWidget->setCurrentIndex(0);
                break;
            case 1:
                current_mode = Mode::Categories;
                ui->mainStackedWidget->setCurrentIndex(0);
                break;
            case 2:
                current_mode = Mode::FocusSessions;
                ui->mainStackedWidget->setCurrentIndex(1);
                break;
            default:
                qDebug() << "Wrong index in comboMode";
        }
        refreshData();
    });
    connect(ui->comboEditCategory, &QComboBox::currentIndexChanged, this, [this](int index){
       active_category_id = ui->comboEditCategory->currentData().toInt();
       refreshCategorySettings();
    });
    connect(ui->btnPickColor, &QPushButton::clicked, this, [this](){
       QColor color = QColorDialog::getColor(Qt::white, this, "Select Category Color", QColorDialog::DontUseNativeDialog);
       if (color.isValid()) {
           ui->lblColorPreview->setProperty("category_color", color.name());
           ui->lblColorPreview->setStyleSheet("QLabel { background-color: " + color.name() + "; border: 1px solid #fff; border-radius: 4px; }");
       }
    });
    connect(ui->btnCreateNewCategory, &QPushButton::clicked, this, [this]() {
        active_category_id = 0;
        ui->comboEditCategory->setPlaceholderText("New Category");
        ui->comboEditCategory->setCurrentIndex(-1);
    });
    connect(ui->btnSaveCategory, &QPushButton::clicked, this, &ActivityDashboard::saveCategorySettings);
    connect(ui->btnDeleteCategory, &QPushButton::clicked, this, &ActivityDashboard::deleteCategory);
    connect(ui->spinWorkTime, &QSpinBox::valueChanged, this, [this](int val){
        timer_remaining_seconds = val * 60;
        updateTimerDisplay();
    });
    connect(ui->btnFocusStart, &QPushButton::clicked, this, &ActivityDashboard::btnFocusStartClicked);
    connect(ui->btnFocusStop, &QPushButton::clicked, this, &ActivityDashboard::btnFocusStopClicked);
    connect(ui->focusTabWidget, &QTabWidget::currentChanged, this, &ActivityDashboard::refreshData);

    updateHiddenAppsGroupBox();

    createMenu();
    tray_icon->setContextMenu(tray_menu);
    QIcon app_icon = QIcon(":/assets/app_icon.png");
    tray_icon->setIcon(app_icon);
    this->setWindowIcon(app_icon);
    connect(tray_icon, &QSystemTrayIcon::activated, this, &ActivityDashboard::iconActivated);
    tray_icon->show();

    focus_timer = new QTimer(this);
    focus_timer->setInterval(1000);
    connect(focus_timer, &QTimer::timeout, this, &ActivityDashboard::onFocusTimerTick);

    updateCategories();
    QMainWindow::showMaximized();
    refreshData();
}

ActivityDashboard::~ActivityDashboard() {
    if (tray_icon) {
        tray_icon->hide();
    }
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
//    qDebug() << "refreshed";
    QDate date_from = ui->dateEditFrom->date();
    QDate date_to = ui->dateEditTo->date();

    if (current_mode == Mode::FocusSessions) {
        if (ui->focusTabWidget->currentIndex() == 1) {
            refreshFocusSessionHistory(date_from, date_to);
        }
    } else {
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
                refreshCategorySettings();
            }
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
        ui->lblProductiveTime->setText("Productive time: 0 min (0%)");
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
    int64_t productive_time = 0;
    for (int i = 0; i < (is_app_mode ? data_apps.size() : data_categories.size()); i++) {
        QString name = is_app_mode ? data_apps[i].display_name : data_categories[i].second.name;
        int64_t total_time = is_app_mode ? data_apps[i].total_time : data_categories[i].first;
        bool is_productive = is_app_mode ? categories[data_apps[i].category_id].is_productive : data_categories[i].second.is_productive;
        if (is_productive) {
            productive_time += total_time;
        }
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

    double productive_percentage = round((double) productive_time / (double) time_sum * 1000) / 10;
    QString productive_metric_str = QString("Productive time: %1 (%2%)")
        .arg(getDisplayTime(productive_time)).arg(productive_percentage);
    ui->lblProductiveTime->setText(productive_metric_str);
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
        refreshCategorySettings();
    }
    ui->tabWidget->setCurrentIndex(3);
}

void ActivityDashboard::refreshCategorySettings() {
    ui->comboEditCategory->blockSignals(true);
    ui->comboEditCategory->clear();
    categories_indexes.clear();
    int ind = 0;
    for(auto &[id, cat] : categories) {
        ui->comboEditCategory->addItem(cat.name, id);
        categories_indexes[id] = ind;
        if (id == active_category_id) {
            ui->comboEditCategory->setCurrentIndex(ind);
        }
        ind++;
    }
    ui->comboEditCategory->blockSignals(false);
    if (ui->comboEditCategory->currentIndex() == -1 && active_category_id == 0) { // New Category
        ui->editCategoryName->setText("");
        QString new_color = generateRandomColor();
        ui->lblColorPreview->setProperty("category_color", new_color);
        ui->lblColorPreview->setStyleSheet("QLabel { background-color: " + new_color + "; border: 1px solid #fff; border-radius: 4px; }");
        ui->checkCatProductive->setCheckState(Qt::CheckState::Checked);
        return;
    }
    if (active_category_id > 0) {
        Category active_category = categories[active_category_id];
        ui->editCategoryName->setText(active_category.name);
        ui->lblColorPreview->setProperty("category_color", active_category.color);
        ui->lblColorPreview->setStyleSheet("QLabel { background-color: " + active_category.color + "; border: 1px solid #fff; border-radius: 4px; }");
        ui->checkCatProductive->setCheckState(active_category.is_productive ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    }
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
    (current_mode == Mode::Categories && active_category_id <= 0)) {
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

void ActivityDashboard::inflictFadingEffectOnLabel(QLabel *label) {
    label->show();
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    label->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(2000);
    a->setStartValue(1);
    a->setEndValue(0);
    a->setEasingCurve(QEasingCurve::InQuad);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(a, &QPropertyAnimation::finished, label, &QLabel::hide);
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
        app_productivity_cache.erase(exe_filename);
        ui->lblSavedStatus->setText("Saved!");
        ui->lblSavedStatus->setStyleSheet("QLabel { color : #4CAF50; }");
        if (active_app.is_hidden != is_hidden) {
            updateHiddenAppsGroupBox();
        }
        active_app.display_name = display_name;
        active_app.category_id = category_id;
        active_app.is_hidden = is_hidden;
    }

    inflictFadingEffectOnLabel(ui->lblSavedStatus);
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

void ActivityDashboard::updateCategories() {
    categories = database_manager->getCategories();
    ui->comboFocusCategory->clear();
    int ind = 0;
    for(auto &[id, cat] : categories) {
        ui->comboFocusCategory->addItem(cat.name, id);
        if (id == 1) {
            ui->comboFocusCategory->setCurrentIndex(ind);
        }
        ind++;
    }
}

QString ActivityDashboard::generateRandomColor() {
    return QColor::fromRgb(QRandomGenerator::global()->generate()).name();
}

void ActivityDashboard::saveCategorySettings() {
    QString category_name = ui->editCategoryName->text();
    QString color = ui->lblColorPreview->property("category_color").toString();
    bool is_productive = ui->checkCatProductive->isChecked();
    if (category_name.isEmpty() || active_category_id == -1) {
        ui->lblCategorySavedStatus->setText("Incorrect input");
        ui->lblCategorySavedStatus->setStyleSheet("QLabel { color : red; }");
    } else {
        if (active_category_id == 0) { // New Category
            int id = database_manager->insertNewCategory({0, category_name, color, is_productive});
            if (id == -1) {
                ui->lblCategorySavedStatus->setText("Something went wrong");
                ui->lblCategorySavedStatus->setStyleSheet("QLabel { color : red; }");
            } else if (id == 0) {
                ui->lblCategorySavedStatus->setText("Category with this name\n already exists");
                ui->lblCategorySavedStatus->setStyleSheet("QLabel { color : orange; }");
            } else {
                active_category_id = id;
                ui->lblCategorySavedStatus->setText("Created!");
                ui->lblCategorySavedStatus->setStyleSheet("QLabel { color : #4CAF50; }");
            }
        } else {
            if (active_category_id == 1 && category_name != "Uncategorized") {
                ui->lblCategorySavedStatus->setText("Renaming \"Uncategorized\"\n is not allowed");
                ui->lblCategorySavedStatus->setStyleSheet("QLabel { color : orange; }");
            } else {
                database_manager->updateCategoryInfo({active_category_id, category_name, color, is_productive});
                app_productivity_cache.clear();
                ui->lblCategorySavedStatus->setText("Saved!");
                ui->lblCategorySavedStatus->setStyleSheet("QLabel { color : #4CAF50; }");
            }
        }
        updateCategories();
    }

    inflictFadingEffectOnLabel(ui->lblCategorySavedStatus);
    refreshCategorySettings();
}

void ActivityDashboard::deleteCategory() {
    if (active_category_id <= 0) {
        ui->lblCategoryDeletedStatus->setText("Category not selected");
        ui->lblCategoryDeletedStatus->setStyleSheet("QLabel { color : red; }");
    } else if (active_category_id == 1) {
        ui->lblCategoryDeletedStatus->setText("Deleting \"Uncategorized\"\n is not allowed");
        ui->lblCategoryDeletedStatus->setStyleSheet("QLabel { color : red; }");
    } else {
        QMessageBox message_box;
        message_box.setIcon(QMessageBox::Question);
        message_box.setText("Confirm Deletion");
        message_box.setInformativeText("Are you sure you want to delete this category?\n\n"
                    "Apps in this category will be moved to \"Uncategorized\".");
        message_box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        message_box.setDefaultButton(QMessageBox::No);
        int ans = message_box.exec();

        if (ans == QMessageBox::Yes) {
            database_manager->deleteCategory(active_category_id);
            updateCategories();
            ui->comboEditCategory->setPlaceholderText("Right click a category from Overview or choose it here");
            ui->comboEditCategory->setCurrentIndex(-1);
            active_category_id = -1;
            ui->lblCategoryDeletedStatus->setText("Deleted!");
            ui->lblCategoryDeletedStatus->setStyleSheet("QLabel { color : #FF9800; }");
        } else {
            return;
        }
    }

    inflictFadingEffectOnLabel(ui->lblCategoryDeletedStatus);
    refreshCategorySettings();
}

void ActivityDashboard::insertFocusSession() {
    int duration = total_session_seconds - timer_remaining_seconds;
    int category_id = ui->comboFocusCategory->currentData().toInt();
    QDateTime date_time = QDateTime::currentDateTime();
    database_manager->insertFocusSession(category_id, date_time, duration);
}

void ActivityDashboard::updateFocusTimerStyle() {
    switch (timer_state) {
        case TimerState::Disabled:
        {
            ui->lblTimerStatus->setText("Ready to focus");
            ui->groupFocusSettings->setEnabled(true);
            ui->frameTimerDisplay->setStyleSheet(
                "QFrame#frameTimerDisplay { background-color: transparent; border: none; }"
                "QLabel { color: #aaaaaa; }"
            );
            ui->btnFocusStart->setText("Start");
            ui->btnFocusStop->setText("Stop");
            ui->btnFocusStop->setEnabled(false);
            ui->btnFocusStart->setStyleSheet("QPushButton { background-color: #8B4513; color: white; border: none; } QPushButton:hover { background-color: #a0522d; }");
            ui->btnFocusStop->setStyleSheet("QPushButton { background-color: #333333; color: #777777; border: none; }");
            break;
        }
        case TimerState::Working:
        {
            ui->groupFocusSettings->setEnabled(false);
            ui->btnFocusStop->setEnabled(true);
            if (!is_break_mode) {
                ui->lblTimerStatus->setText("Focusing...");
                ui->frameTimerDisplay->setStyleSheet(
                    "QFrame#frameTimerDisplay { background-color: rgba(255, 78, 33, 0.15); border: 2px solid rgba(139, 69, 19, 0.4); }"
                    "QLabel { color: #e6b89c; }"
                );
                ui->btnFocusStart->setText("Pause");
                ui->btnFocusStop->setText("Stop");
                ui->btnFocusStart->setStyleSheet("QPushButton { background-color: rgba(122, 122, 122, 0.7); color: white; } QPushButton:hover { background-color: #8f8f8f; }");
                ui->btnFocusStop->setStyleSheet("QPushButton { background-color: rgba(220, 53, 69, 0.8); color: white; border: none; } QPushButton:hover { background-color: #dc3545; }");
            } else {
                ui->lblTimerStatus->setText("Break time");
                ui->frameTimerDisplay->setStyleSheet(
                    "QFrame#frameTimerDisplay { background-color: rgba(25, 135, 84, 0.15); border: 2px solid rgba(25, 135, 84, 0.4); }"
                    "QLabel { color: #20c997; }"
                );
                ui->btnFocusStart->setText("Pause");
                ui->btnFocusStop->setText("Skip");
                ui->btnFocusStart->setStyleSheet("QPushButton { background-color: rgba(122, 122, 122, 0.7); color: white; } QPushButton:hover { background-color: #8f8f8f; }");
                ui->btnFocusStop->setStyleSheet("QPushButton { background-color: rgba(108, 117, 125, 0.6); color: white; border: none; } QPushButton:hover { background-color: #6c757d; }");
            }
            break;
        }
        case TimerState::Paused:
        {
            ui->lblTimerStatus->setText("Paused");
            ui->btnFocusStart->setText("Resume");
            ui->btnFocusStop->setText(is_break_mode ? "Skip" : "Done");
            if (!is_break_mode) {
                ui->btnFocusStart->setStyleSheet("QPushButton { background-color: #8B4513; color: white; border: none; } QPushButton:hover { background-color: #a0522d; }");
                ui->btnFocusStop->setStyleSheet("QPushButton { background-color: #198754; color: white; border: none; } QPushButton:hover { background-color: #157347; }");
            } else {
                ui->btnFocusStart->setStyleSheet("QPushButton { background-color: #198754; color: white; border: none; } QPushButton:hover { background-color: #157347; }");
                ui->btnFocusStop->setStyleSheet("QPushButton { background-color: #6c757d; color: white; border: none; } QPushButton:hover { background-color: #5a6268; }");
            }
            break;
        }
    }
}

void ActivityDashboard::updateTimerDisplay() {
    int mins = timer_remaining_seconds / 60;
    int secs = timer_remaining_seconds % 60;

    ui->lblTimerDisplay->setText(QString("%1:%2").arg(mins, 2, 10, '0').arg(secs, 2, 10, '0'));
}

void ActivityDashboard::onFocusTimerTick() {
    timer_remaining_seconds--;
    updateTimerDisplay();
    if (timer_remaining_seconds <= 0) {
        focus_timer->stop();
        if (is_break_mode) {
            timer_state = TimerState::Disabled;
            timer_remaining_seconds = ui->spinWorkTime->value() * 60;
            updateTimerDisplay();
            QIcon message_icon = QApplication::style()->standardIcon(QStyle::SP_DialogNoButton);
            tray_icon->showMessage("Break is over", "The next pomodoro will go better!", message_icon, 5000);
        } else {
            emit toggleFocusSessionFlag(false);
            insertFocusSession();
            timer_remaining_seconds = ui->spinBreakTime->value() * 60;
            focus_timer->start();
            updateTimerDisplay();
            QIcon message_icon = QApplication::style()->standardIcon(QStyle::SP_DialogYesButton);
            tray_icon->showMessage("Pomodoro is finished", "Take a break", message_icon, 5000);
        }
        is_break_mode = !is_break_mode;
        updateFocusTimerStyle();
    }
}

void ActivityDashboard::btnFocusStartClicked() {
    switch (timer_state) {
        case TimerState::Disabled: {
            timer_remaining_seconds = ui->spinWorkTime->value() * 60;
            total_session_seconds = timer_remaining_seconds;
            timer_state = TimerState::Working;
            emit toggleFocusSessionFlag(true);
            is_break_mode = false;
            updateFocusTimerStyle();
            focus_timer->start();
            break;
        }
        case TimerState::Working: {
            focus_timer->stop();
            timer_state = TimerState::Paused;
            if (!is_break_mode) {
                emit toggleFocusSessionFlag(false);
            }
            updateFocusTimerStyle();
            break;
        }
        case TimerState::Paused: {
            focus_timer->start();
            timer_state = TimerState::Working;
            if (!is_break_mode) {
                emit toggleFocusSessionFlag(true);
            }
            updateFocusTimerStyle();
            break;
        }
    }
}

void ActivityDashboard::btnFocusStopClicked() {
    if (timer_state == TimerState::Paused && !is_break_mode) {
        insertFocusSession();
        timer_state = TimerState::Working;
        is_break_mode = true;
        timer_remaining_seconds = ui->spinBreakTime->value() * 60;
        focus_timer->start();
        QIcon message_icon = QApplication::style()->standardIcon(QStyle::SP_DialogYesButton);
        tray_icon->showMessage("Pomodoro is finished", "Take a break", message_icon, 5000);
    } else {
        focus_timer->stop();
        timer_state = TimerState::Disabled;
        if (!is_break_mode) {
            emit toggleFocusSessionFlag(false);
        }
        timer_remaining_seconds = ui->spinWorkTime->value() * 60;
        is_break_mode = false;
    }
    updateTimerDisplay();
    updateFocusTimerStyle();
}

void ActivityDashboard::refreshFocusSessionHistory(QDate &date_from, QDate &date_to) {
    std::vector<std::pair<int, int64_t>> sessions_categories = database_manager->getFocusSessionsCategoriesStats(date_from, date_to);
    std::vector<FocusSession> focus_sessions = database_manager->getFocusSessions(date_from, date_to);
    int64_t time_sum = std::accumulate(sessions_categories.begin(), sessions_categories.end(), (int64_t) 0,
                                       [](auto& a, auto& b) {
                                           return a + b.second;
                                       });;

    ui->lblTotalFocusStat->setText(getDisplayTime(time_sum));
    ui->lblTotalSessionsStat->setText(QString::number(focus_sessions.size()));
    ui->tableFocusHistory->setRowCount(focus_sessions.size());

    QChart* chart = ui->chartPieFocus->chart();
    if (!chart) {
        chart = new QChart;
        ui->chartPieFocus->setChart(chart);
        ui->chartPieFocus->setRenderHint(QPainter::Antialiasing);
    }
    chart->removeAllSeries();

    auto empty_data_label = ui->chartPieFocus->findChild<QLabel*>("emptyDataLabelFocus");
    if (sessions_categories.empty()) {
        if (!empty_data_label) {
            empty_data_label = new QLabel("No data for these dates", ui->chartPieFocus);
            empty_data_label->setObjectName("emptyDataLabelFocus");
            empty_data_label->setAlignment(Qt::AlignCenter);
            empty_data_label->setStyleSheet("QLabel { color : orange; font-size : 20px; }");

            QVBoxLayout* overlay_layout = new QVBoxLayout(ui->chartPieFocus);
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
    for (int i = 0; i < sessions_categories.size(); i++) {
        Category category = categories[sessions_categories[i].first];
        QString name = category.name;
        int64_t total_time = sessions_categories[i].second;
        QPieSlice *slice = new QPieSlice(name, total_time);
        slice->setColor(category.color);
        if (total_time * 20 >= time_sum) {
            pie_series->append(slice);
        } else {
            others_time += total_time;
        }
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

    for (int i = 0; i < focus_sessions.size(); i++) {
        QDateTime date_time = QDateTime::fromSecsSinceEpoch(focus_sessions[i].session_datetime);
        QTableWidgetItem *date_item = new QTableWidgetItem(date_time.toString("d MMMM yyyy hh:mm:ss"));
        ui->tableFocusHistory->setItem(i, 0, date_item);
        QTableWidgetItem *category_item = new QTableWidgetItem(categories[focus_sessions[i].category_id].name);
        ui->tableFocusHistory->setItem(i, 1, category_item);
        QTableWidgetItem *duration_item = new QTableWidgetItem(getDisplayTime(focus_sessions[i].duration));
        ui->tableFocusHistory->setItem(i, 2, duration_item);
    }
}

void ActivityDashboard::currentAppChanged(const QString &exe_filename) {
    if (!ui->productivityNotificationsCheckBox->isChecked()) return;

    if (timer_state == TimerState::Working) {
        bool is_productive;
        if (app_productivity_cache.find(exe_filename) != app_productivity_cache.end()) {
            is_productive = app_productivity_cache[exe_filename];
        } else {
            is_productive = database_manager->isAppProductive(exe_filename);
            app_productivity_cache[exe_filename] = is_productive;
        }

        if (!is_productive) {
            int rnd_idx = QRandomGenerator::global()->bounded(tray_messages.size());
            QIcon message_icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
            tray_icon->showMessage(tray_messages[rnd_idx].first, tray_messages[rnd_idx].second, message_icon, 5000);
        }
    }
}
