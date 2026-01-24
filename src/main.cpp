// TODO: прикрутить базу данных, сделать обработчик сигналов (Ctrl+C и тд), добавить треды
// TODO: добавить пользователям возможность сменить имя программы

#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fcntl.h>

struct WindowData {
    HWND window_handle = nullptr;
    DWORD process_id = 0;
    std::wstring window_title;
    HANDLE process_handle = nullptr;
    std::wstring exe_filename;
    std::string error;

    bool isValid() const {
        return !exe_filename.empty();
    }
};

WindowData getWindowData() {
    const int string_max_length = 1024;
    WindowData window_data;
    wchar_t buffer[string_max_length] = {0};

    window_data.window_handle = GetForegroundWindow();
    if (!window_data.window_handle) {
        window_data.error = "GetForegroundWindow error";
        return window_data;
    }
    if (GetWindowTextW(window_data.window_handle, buffer, string_max_length) > 0) {
        window_data.window_title = buffer;
    }
    LPDWORD process_id_ptr = &window_data.process_id;
    GetWindowThreadProcessId(window_data.window_handle, process_id_ptr);
    if (!window_data.process_id) {
        window_data.error = "GetWindowThreadProcessId error";
        return window_data;
    }
    window_data.process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                             false, window_data.process_id);
    if (window_data.process_handle) {
        DWORD image_name_size = string_max_length;
        if (!QueryFullProcessImageNameW(window_data.process_handle, 0,
                                   buffer, &image_name_size)) {
            window_data.error = "QueryFullProcessImageNameW error";
        }
        window_data.exe_filename = buffer;
        CloseHandle(window_data.process_handle);
    } else {
        window_data.error = "OpenProcess error";
    }
    return window_data;
}

void printWindowData(WindowData& window_data) {
    std::wcout << L"Handle: " << window_data.window_handle << std::endl;
    std::wcout << L"Process Id: " << window_data.process_id << std::endl;
    std::wcout << L"Window Title: " << window_data.window_title << std::endl;
    std::wcout << L"Window Process Handle: " << window_data.process_handle << std::endl;
    std::wcout << L"Window Exe Filename: " << window_data.exe_filename << std::endl;
    std::wcout << std::endl;
}

int main() {
    setlocale(LC_ALL, "");
    _setmode(_fileno(stdout), _O_U16TEXT);

    WindowData window_data;
    do {
        window_data = getWindowData();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    } while (!window_data.isValid());
    printWindowData(window_data);

    while (true) {
        WindowData new_window_data = getWindowData();
        if (new_window_data.isValid() && window_data.window_handle != new_window_data.window_handle) {
            window_data = new_window_data;
            printWindowData(window_data);
        } else if (!new_window_data.isValid()) {
            std::cerr << new_window_data.error << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}