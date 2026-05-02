// TODO: сделать обработку вкладок браузера (в том числе обновить цикл в main)
// TODO: при отвлечении во время активной фокус сессии показывать сообщения с юмором, а не пустые "Вы отвлеклись"
// TODO: создать подсказку для пользователя, что можно создать доп категорию с припиской отвлекающая,
// TODO: чтобы если что в категории с одним названием были и отвлекающие приложения, и нет

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

#include "WindowData.h"
#include "DatabaseManager.h"
#include "WindowsReaderThread.h"
#include "ActivityDashboard.h"

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
    QSpinBox::up-button, QSpinBox::down-button {
        background-color: #383838;
        border-radius: 2px;
        width: 20px;
    }
    QSpinBox::up-button:hover, QSpinBox::down-button:hover {
        background-color: #0D6EFD;
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
    )";

    a.setStyleSheet(styleSheet);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("Activity Insight");
    setupAppStyle(a);

//    setlocale(LC_ALL, "");
//    _setmode(_fileno(stdout), _O_U16TEXT);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");

    qRegisterMetaType<WindowData>("WindowData");

    DatabaseManager database_manager;
    database_manager.init();

    ActivityDashboard activity_dashboard(&database_manager);
    WindowsReaderThread windows_reader_thread(&activity_dashboard);


    QObject::connect(&windows_reader_thread, &WindowsReaderThread::sendActivityLog, &database_manager,
            [&database_manager](const WindowData& window_data) {
        database_manager.insertActivityLog(window_data);
        for (auto& x : database_manager.getUpdatedDailyAppStats()) {
            std::wcout << x.display_name.toStdWString() << L": " << x.total_time << L" sec" << std::endl;
        }
        std::wcout << std::endl;
    });

    windows_reader_thread.start();
    activity_dashboard.show();

    auto exit_code = a.exec();

    windows_reader_thread.requestInterruption();
    windows_reader_thread.wait();

    return exit_code;
}