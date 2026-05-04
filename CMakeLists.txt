cmake_minimum_required(VERSION 3.26)
project(Activity_Insight)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_executable(Activity_Insight src/main.cpp
        src/DatabaseManager.h
        src/DatabaseManager.cpp
        src/AppStats.h
        src/WindowsReaderThread.h
        src/WindowsReaderThread.cpp
        src/ActivityDashboard.cpp
        src/ActivityDashboard.h
        src/ActivityDashboard.ui
        assets/app.rc
        src/Category.h
        src/Mode.h
        src/TimerState.h
        src/FocusSession.h
)

target_link_libraries(Activity_Insight PRIVATE version)

find_package(Qt6 REQUIRED COMPONENTS Sql Core Widgets Charts)
target_link_libraries(Activity_Insight PRIVATE Qt6::Sql Qt6::Core Qt6::Widgets Qt6::Charts)

set(app_resource_files
    "assets/app_icon.png"
)

qt_add_resources(Activity_Insight "app_resources"
    PREFIX "/"
    FILES ${app_resource_files}
)
