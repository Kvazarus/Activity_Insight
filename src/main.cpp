// TODO: сделать обработчик сигналов (Ctrl+C и тд), добавить треды
// TODO: добавить пользователям возможность сменить имя программы
// TODO: сделать обработку вкладок браузера (в том числе обновить цикл в main)
// TODO: добавить GetAncestor для повышения вероятности успешного считывания окна (а может и не надо, пока всё работает и без него)
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

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

//    setlocale(LC_ALL, "");
//    _setmode(_fileno(stdout), _O_U16TEXT);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");

    qRegisterMetaType<WindowData>("WindowData");

    ActivityDashboard activity_dashboard;
    WindowsReaderThread windows_reader_thread(&activity_dashboard);
    DatabaseManager database_manager(&activity_dashboard);
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