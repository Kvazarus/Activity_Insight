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

// TODO: Добавить проверку на IDLE

// TODO: после добавления таблицы для вкладок браузера, поменять сравнения на window_title, а не window_handle
// Возможно стоит игнорировать системные окна по типу Рабочего стола или переключателя окон (это всё explorer.exe)
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
    void run() override;
 signals:
    void sendActivityLog(const WindowData& window_data);
};

#endif //ACTIVITY_INSIGHT_SRC_WINDOWSREADERTHREAD_H_
