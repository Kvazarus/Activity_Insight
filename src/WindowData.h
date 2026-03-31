#ifndef ACTIVITY_INSIGHT_WINDOW_DATA
#define ACTIVITY_INSIGHT_WINDOW_DATA

#include <windows.h>

struct WindowData {
    HWND window_handle = nullptr;
    DWORD process_id = 0;
    std::wstring window_title;
    std::wstring exe_filename;
    std::wstring display_name;
    std::string error;
    int64_t time = 0;

    bool isValid() const {
        return !exe_filename.empty();
    }
};

#endif //ACTIVITY_INSIGHT_WINDOW_DATA
