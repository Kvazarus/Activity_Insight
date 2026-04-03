#include "WindowsReaderThread.h"

std::wstring WindowsReaderThread::getAppNameFromPath(const std::wstring &exe_filename) {
    return std::filesystem::path(exe_filename).stem().wstring();
}

std::wstring WindowsReaderThread::getAppName(const std::wstring &exe_filename) {
    DWORD dummy = 0;
    uint32_t buffer_size = GetFileVersionInfoSizeW(exe_filename.c_str(), &dummy);
    if (!buffer_size) return getAppNameFromPath(exe_filename);
    std::vector<BYTE> buffer(buffer_size);
    if (!GetFileVersionInfoW(exe_filename.c_str(), 0, buffer_size, buffer.data()))
        return getAppNameFromPath(exe_filename);

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

        if (VerQueryValueW(buffer.data(), lang_and_codepage_str, (LPVOID *) &app_name, &app_data_size)
            && app_data_size > 0) {
            return std::wstring(app_name);
        }
    }

    return getAppNameFromPath(exe_filename);
}

WindowData WindowsReaderThread::getWindowData() {
    const int string_max_length = 1024;
    WindowData window_data;
    wchar_t window_text_buffer[string_max_length] = {0};

    window_data.window_handle = GetForegroundWindow();
    if (!window_data.window_handle) {
        window_data.error = "GetForegroundWindow error";
        return window_data;
    }
    if (GetWindowTextW(window_data.window_handle, window_text_buffer, string_max_length) > 0) {
        window_data.window_title = window_text_buffer;
    }
    LPDWORD process_id_ptr = &window_data.process_id;
    GetWindowThreadProcessId(window_data.window_handle, process_id_ptr);
    if (!window_data.process_id) {
        window_data.error = "GetWindowThreadProcessId error";
        return window_data;
    }
    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                        false, window_data.process_id);
    if (process_handle) {
        wchar_t image_name_buffer[string_max_length] = {0};
        DWORD image_name_size = string_max_length;
        if (!QueryFullProcessImageNameW(process_handle, 0,
                                        image_name_buffer, &image_name_size)) {
            window_data.error = "QueryFullProcessImageNameW error";
        }
        window_data.exe_filename = image_name_buffer;
        CloseHandle(process_handle);
    } else {
        window_data.error = "OpenProcess error";
    }

    if (app_name_cache.count(window_data.exe_filename) == 0) {
        app_name_cache[window_data.exe_filename] = getAppName(window_data.exe_filename);
    }
    window_data.display_name = app_name_cache[window_data.exe_filename];

    return window_data;
}

void WindowsReaderThread::printWindowData(WindowData &window_data) {
    std::wcout << L"Handle: " << window_data.window_handle << std::endl;
    std::wcout << L"Process Id: " << window_data.process_id << std::endl;
    std::wcout << L"Window Title: " << window_data.window_title << std::endl;
    std::wcout << L"Window Exe Filename: " << window_data.exe_filename << std::endl;
    std::wcout << L"App Name: " << window_data.display_name << std::endl;
    std::wcout << std::endl;
}

int64_t WindowsReaderThread::getTimeDiffInSecs(std::chrono::time_point<std::chrono::steady_clock> begin,
                          std::chrono::time_point<std::chrono::steady_clock> end) {
    return std::chrono::duration_cast<std::chrono::seconds>(abs(end - begin)).count();
}