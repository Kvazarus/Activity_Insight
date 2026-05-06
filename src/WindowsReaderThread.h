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
#include <atomic>

#include <QThread>

#include "WindowData.h"

class WindowsReaderThread : public QThread {
    Q_OBJECT
 private:
    std::unordered_map<std::wstring, std::wstring> app_name_cache;
    std::atomic<bool> is_focus_session = false;
    const int64_t idle_milleseconds_threshold = 7*60*1000; // 7 minutes
    bool was_user_idle = false;

    std::wstring getAppNameFromPath(const std::wstring &exe_filename);
    std::wstring getAppName(const std::wstring &exe_filename);
    WindowData getWindowData();
    void printWindowData(WindowData &window_data);
    int64_t getTimeDiffInSecs(std::chrono::time_point<std::chrono::steady_clock> begin,
                                                   std::chrono::time_point<std::chrono::steady_clock> end);
    bool isUserIDLE();

 public:
    explicit WindowsReaderThread(QObject *parent = nullptr) : QThread(parent) {}
 protected:
    void run() override;
 signals:
    void sendActivityLog(const WindowData& window_data);
    void sendCurrentApp(const QString& exe_filename);

 public slots:
    void focusSessionFlagToggled(bool is_focus_session_);
};

#endif //ACTIVITY_INSIGHT_SRC_WINDOWSREADERTHREAD_H_
