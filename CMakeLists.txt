cmake_minimum_required(VERSION 3.26)
project(Activity_Insight)

set(CMAKE_CXX_STANDARD 17)

add_executable(Activity_Insight src/main.cpp
        src/DatabaseManager.h)

target_link_libraries(Activity_Insight PRIVATE version)

find_package(Qt6 REQUIRED COMPONENTS Sql)
target_link_libraries(Activity_Insight PRIVATE Qt6::Sql)
