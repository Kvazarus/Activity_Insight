// TODO: прикрутить базу данных, сделать обработчик сигналов (Ctrl+C и тд), добавить треды
// TODO: добавить пользователям возможность сменить имя программы

#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fcntl.h>
#include <vector>
#include <strsafe.h>

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

std::wstring getAppNameFromPath(const std::wstring &exe_filename) {
    int pos = 0;
    for (int i = 0; i < exe_filename.size(); i++) {
        if (exe_filename[i] == '\\') pos = i + 1;
    }
    return exe_filename.substr(pos, exe_filename.size() - pos - 4);
}

std::wstring getAppName(const std::wstring &exe_filename) {
    DWORD dummy = 0;
    uint32_t buffer_size = GetFileVersionInfoSizeW(exe_filename.c_str(), &dummy);
    if (!buffer_size) return getAppNameFromPath(exe_filename);
    std::vector<BYTE> buffer(buffer_size);
    if (!GetFileVersionInfoW(exe_filename.c_str(), 0, buffer_size, buffer.data())) return getAppNameFromPath(exe_filename);

    HRESULT hr;
    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    };
    LANGANDCODEPAGE *lpTranslate;

    uint32_t data_size = 0;
    VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation",
                   (LPVOID *) &lpTranslate, &data_size);
    if (!data_size) return getAppNameFromPath(exe_filename);
    for (int i = 0; i < (data_size / sizeof(LANGANDCODEPAGE)); i++) {
        wchar_t lang_and_codepage_str[100];
        hr = StringCchPrintfW(lang_and_codepage_str, 100, L"\\StringFileInfo\\%04x%04x\\FileDescription",
                             lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
        if (FAILED(hr)) continue;

        wchar_t *app_name = nullptr;
        uint32_t app_data_size = 0;

        if (VerQueryValueW(buffer.data(), lang_and_codepage_str, (LPVOID*) &app_name, &app_data_size) && app_data_size > 0) {
            return std::wstring(app_name);
        }
    }

    return getAppNameFromPath(exe_filename);
}

void printWindowData(WindowData &window_data) {
    std::wcout << L"Handle: " << window_data.window_handle << std::endl;
    std::wcout << L"Process Id: " << window_data.process_id << std::endl;
    std::wcout << L"Window Title: " << window_data.window_title << std::endl;
    std::wcout << L"Window Process Handle: " << window_data.process_handle << std::endl;
    std::wcout << L"Window Exe Filename: " << window_data.exe_filename << std::endl;

    // TODO: закешировать app name

    std::wcout << L"App Name: " << getAppName(window_data.exe_filename) << std::endl;
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