#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fcntl.h>
#include <vector>
#include <strsafe.h>
#include <filesystem>
#include <corecrt_io.h>

#include <QApplication>
#include <QMessageBox>

#include "WindowData.h"
#include "DatabaseManager.h"
#include "WindowsReaderThread.h"
#include "ActivityDashboard.h"
#include "RunGuard.h"

// TODO: сделать приложение Single Instance (то бишь когда запускаешь второй раз приложение, оно закрывалось само)

void setupAppStyle(QApplication& a) {
    a.setStyle("Fusion");

    QString styleSheet = R"(
    /* --- Buttons --- */
    QPushButton {
        background-color: #0D6EFD;
        color: white;
        border-radius: 6px;
        padding: 8px 16px;
        font-weight: bold;
        border: none;
    }
    QPushButton:hover {
        background-color: #0B5ED7;
    }
    QPushButton:pressed {
        background-color: #0a53be;
    }

    /* --- Delete Button Override --- */
    QPushButton#btnDeleteCategory {
        background-color: #dc3545;
    }
    QPushButton#btnDeleteCategory:hover {
        background-color: #c82333;
    }
    QPushButton#btnDeleteCategory:pressed {
        background-color: #bd2130;
    }

    /* --- Tables --- */
    QTableWidget {
        background-color: #1e1e1e;
        color: #ffffff;
        border: 1px solid #444;
        border-radius: 6px;
        gridline-color: #333;
        selection-background-color: #0D6EFD;
    }
    QHeaderView::section {
        background-color: #2b2b2b;
        color: #cccccc;
        padding: 6px;
        border: none;
        border-bottom: 2px solid #0D6EFD;
        font-weight: bold;
    }
    QTableWidget::item {
        padding: 4px;
    }

    /* --- Tabs --- */
    QTabWidget::pane {
        border: 1px solid #444;
        border-radius: 6px;
        background-color: #1e1e1e;
        top: -1px;
    }
    QTabBar::tab {
        background-color: #2b2b2b;
        color: #aaaaaa;
        padding: 8px 20px;
        border-top-left-radius: 6px;
        border-top-right-radius: 6px;
        margin-right: 2px;
        border: 1px solid transparent;
    }
    QTabBar::tab:selected {
        background-color: #1e1e1e;
        color: #ffffff;
        border: 1px solid #444;
        border-bottom: none;
        border-top: 3px solid #0D6EFD;
    }
    QTabBar::tab:hover:!selected {
        background-color: #383838;
        color: #ffffff;
    }

    /* --- Date Edits --- */
    QDateEdit {
        background-color: #2b2b2b;
        color: white;
        border: 1px solid #444;
        border-radius: 4px;
        padding: 4px 8px;
    }
    QDateEdit::drop-down {
        border-left: 1px solid #444;
        width: 20px;
    }

    /* --- Splitter --- */
    QSplitter::handle {
        background-color: #444;
        margin: 10px 2px;
        border-radius: 2px;
    }

    /* --- ChartView --- */
    QChartView {
        background-color: transparent;
        border: none;
    }

    /* --- LineEdits and ComboBoxes --- */
    QLineEdit, QComboBox {
        background-color: #2b2b2b;
        color: white;
        border: 1px solid #444;
        border-radius: 4px;
        padding: 4px;
    }

    /* --- Group Box --- */
    QGroupBox {
        background-color: transparent;
        border: 1px solid #444;
        border-radius: 6px;
        margin-top: 18px;
        padding-top: 15px;
        padding-bottom: 5px;
        font-size: 16px;
        font-weight: bold;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top center;
        padding: 0 10px;
    }

    /* --- List Widget (Hidden Apps) --- */
    QListWidget {
        background-color: #2b2b2b;
        color: #ffffff;
        border: 1px solid #444;
        border-radius: 4px;
        padding: 5px;
        font-size: 13px;
        outline: none;
    }
    QListWidget::item {
        padding: 8px 12px;
        border-radius: 4px;
        margin-bottom: 2px;
    }
    QListWidget::item:hover {
        background-color: #383838;
    }
    QListWidget::item:selected {
        background-color: #0D6EFD;
        color: white;
    }

    /* --- ScrollBars --- */
    QScrollBar:vertical {
        border: none;
        background: #1e1e1e;
        width: 10px;
        margin: 0px 0px 0px 0px;
    }
    QScrollBar::handle:vertical {
        background: #555;
        min-height: 20px;
        border-radius: 5px;
    }
    QScrollBar::handle:vertical:hover {
        background: #777;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
    }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
        background: none;
    }

    /* --- SpinBoxes --- */
    QSpinBox {
        background-color: #2b2b2b;
        color: white;
        border: 1px solid #444;
        border-radius: 4px;
        padding: 4px 8px;
        font-size: 14px;
    }

    /* --- Focus Timer Buttons --- */
    QPushButton#btnFocusStart {
        background-color: #198754;
    }
    QPushButton#btnFocusStart:hover {
        background-color: #157347;
    }
    QPushButton#btnFocusStart:pressed {
        background-color: #146c43;
    }

    QPushButton#btnFocusStop {
        background-color: #dc3545;
    }
    QPushButton#btnFocusStop:hover {
        background-color: #c82333;
    }
    QPushButton#btnFocusStop:pressed {
        background-color: #bd2130;
    }

    /* --- CheckBoxes --- */
    QCheckBox {
        color: #ffffff;
        spacing: 8px;
    }

    QCheckBox::indicator {
        width: 18px;
        height: 18px;
        background-color: #2b2b2b;
        border: 1px solid #444;
        border-radius: 4px;
    }

    QCheckBox::indicator:hover {
        border: 1px solid #0D6EFD;
    }

    QCheckBox::indicator:checked {
        background-color: #0D6EFD;
        border: 1px solid #0D6EFD;
        image: url(:/assets/check_icon.png);
    }

    QCheckBox::indicator:checked:hover {
        background-color: #0B5ED7;
        border: 1px solid #0B5ED7;
    }
    )";

    a.setStyleSheet(styleSheet);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    RunGuard guard("ActivityInsightApp");
    if (!guard.tryToRun()) {
        QMessageBox::information(nullptr, "Application Already Running",
                              "Activity Insight is already running. Please check your system tray");
        return 0;
    }

    a.setApplicationName("Activity Insight");
    setupAppStyle(a);

//    setlocale(LC_ALL, "");
//    _setmode(_fileno(stdout), _O_U16TEXT);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");

    qRegisterMetaType<WindowData>("WindowData");

    DatabaseManager database_manager;
    try {
        database_manager.init();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Database Error",
                              QString("Failed to initialize the database:\n%1").arg(e.what()));
        return -1;
    }

    ActivityDashboard activity_dashboard(&database_manager);
    WindowsReaderThread windows_reader_thread(&activity_dashboard);


    QObject::connect(&windows_reader_thread, &WindowsReaderThread::sendActivityLog, &database_manager,
            [&database_manager](const WindowData& window_data) {
        database_manager.insertActivityLog(window_data);
//        for (auto& x : database_manager.getUpdatedDailyAppStats()) {
//            std::wcout << x.display_name.toStdWString() << L": " << x.total_time << L" sec" << std::endl;
//        }
//        std::wcout << std::endl;
    });
    QObject::connect(&windows_reader_thread, &WindowsReaderThread::sendCurrentApp, &activity_dashboard, &ActivityDashboard::currentAppChanged);
    QObject::connect(&activity_dashboard, &ActivityDashboard::toggleFocusSessionFlag, &windows_reader_thread, &WindowsReaderThread::focusSessionFlagToggled);

    windows_reader_thread.start();
    activity_dashboard.show();

    auto exit_code = a.exec();

    windows_reader_thread.requestInterruption();
    windows_reader_thread.wait();

    return exit_code;
}