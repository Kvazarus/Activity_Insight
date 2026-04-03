#ifndef ACTIVITY_INSIGHT_SRC_WINDOWSREADERTHREAD_H_
#define ACTIVITY_INSIGHT_SRC_WINDOWSREADERTHREAD_H_

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

#include <QThread>

#include "WindowData.h"

// TODO: после добавления таблицы для вкладок браузера, поменять сравнения на window_title, а не window_handle
// Возможно стоит игнорировать системные окна по типу Рабочего стола или переключателя окон (shift-tab)
// В основном из-за того, что может появиться сообщение о непродуктивном приложении в сессии
class WindowsReaderThread : public QThread {
    Q_OBJECT
 private:
    std::unordered_map<std::wstring, std::wstring> app_name_cache;

    std::wstring getAppNameFromPath(const std::wstring &exe_filename);
    std::wstring getAppName(const std::wstring &exe_filename);
    WindowData getWindowData();
    void printWindowData(WindowData &window_data);
    int64_t getTimeDiffInSecs(std::chrono::time_point<std::chrono::steady_clock> begin,
                                                   std::chrono::time_point<std::chrono::steady_clock> end);
 public:
    explicit WindowsReaderThread(QObject *parent = nullptr) : QThread(parent) {}
 protected:
    void run() override {
        WindowData window_data;
        do {
            window_data = getWindowData();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        } while (!window_data.isValid());
        printWindowData(window_data);

        auto begin_time = std::chrono::high_resolution_clock::now();
        while (true) {
            WindowData new_window_data = getWindowData();
            auto end_time = std::chrono::high_resolution_clock::now();
            if (new_window_data.isValid() && window_data.window_handle != new_window_data.window_handle) {
                window_data.time = getTimeDiffInSecs(begin_time, end_time);
                begin_time = end_time;
                emit sendActivityLog(window_data);
                window_data = new_window_data;
                printWindowData(window_data);
            } else if (!new_window_data.isValid()) {
                std::cerr << new_window_data.error << std::endl;
            }
            int64_t time_diff = getTimeDiffInSecs(begin_time, end_time);
            if (time_diff >= 60) {
                window_data.time = time_diff;
                begin_time = end_time;
                emit sendActivityLog(window_data);
            }
            sleep(1);
        }
    }
 signals:
    void sendActivityLog(const WindowData& window_data);
};

#endif //ACTIVITY_INSIGHT_SRC_WINDOWSREADERTHREAD_H_
