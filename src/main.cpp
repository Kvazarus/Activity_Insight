// TODO: сделать обработчик сигналов (Ctrl+C и тд), добавить треды
// TODO: добавить пользователям возможность сменить имя программы (окон)
// TODO: сделать обработку вкладок браузера (в том числе обновить цикл в main)
// TODO: дать возможность юзеру выбирать отрезок времени для просмотра активности в пределе месяца (может и больше месяца)
// TODO: при отвлечении во время активной фокус сессии показывать сообщения с юмором, а не пустые "Вы отвлеклись"
// TODO: создать подсказку для пользователя, что можно создать доп категорию с припиской отвлекающая,
// TODO: чтобы если что в категории с одним названием были и отвлекающие приложения, и нет
// TODO: добавить приложение в систем трей

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
    ActivityDashboard activity_dashboard(&database_manager);
    WindowsReaderThread windows_reader_thread(&activity_dashboard);

    database_manager.init();

    QObject::connect(&windows_reader_thread, &WindowsReaderThread::sendActivityLog, &database_manager,
            [&database_manager](const WindowData& window_data) {
        database_manager.insertActivityLog(window_data);
        for (auto& x : database_manager.getUpdatedDailyAppStats()) {
            std::wcout << x.display_name << L": " << x.total_time << L" sec" << std::endl;
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