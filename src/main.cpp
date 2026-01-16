#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>


struct WindowData {
    HWND handle = nullptr;
    DWORD process_id = 0;
    std::wstring window_title;

    bool isValid() {
        return handle != nullptr;
    }
};

WindowData getWindowData() {
    const int string_max_length = 1024;

    WindowData window_data;
    window_data.handle = GetForegroundWindow();
    if (window_data.handle) {
        wchar_t buffer[string_max_length];
        if (GetWindowTextW(window_data.handle, buffer, string_max_length) > 0) {
            window_data.window_title = buffer;
        }
        LPDWORD process_id_ptr = &window_data.process_id;
        GetWindowThreadProcessId(window_data.handle, process_id_ptr);
    }
    return window_data;
}

void printWindowData(WindowData& window_data) {
    std::wcout << L"Handle: " << window_data.handle << std::endl;
    std::wcout << L"Process Id: " << window_data.process_id << std::endl;
    std::wcout << L"Window Title: " << window_data.window_title << std::endl;
    std::wcout << std::endl;
}

int main() {
    setlocale(LC_ALL, "");

    WindowData window_data = getWindowData();
    printWindowData(window_data);
    while (true) {
        WindowData new_window_data = getWindowData();
        if (new_window_data.isValid() && window_data.handle != new_window_data.handle) {
            window_data = new_window_data;
            printWindowData(window_data);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}