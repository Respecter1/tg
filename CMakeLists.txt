cmake_minimum_required(VERSION 3.10)
project(untitled)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/home/respecter8/vcpkg/installed/x64-linux")

find_package(Boost REQUIRED COMPONENTS system)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_library(SQLiteCpp_LIB NAMES SQLiteCpp PATHS CMAKE_PREFIX_PATH)
find_library(SQLite3_LIB NAMES sqlite3 PATHS CMAKE_PREFIX_PATH)
find_library(TGBOT_LIBRARY NAMES libTgBot.a PATHS CMAKE_PREFIX_PATH)

add_executable(untitled main.cpp)


target_link_libraries(untitled
        ${TGBOT_LIBRARY}
        ${Boost_LIBRARIES}
        ${SQLiteCpp_LIB}
        ${SQLite3_LIB}
        cpr::cpr
)