cmake_minimum_required(VERSION 3.14)
project(OPer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к локальному архиву (относительно текущего файла)
set(JSON_LOCAL_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/external/json.tar.xz")

include(FetchContent)

if(EXISTS "${JSON_LOCAL_ARCHIVE}")
    message(STATUS "Using local json.tar.xz from ${JSON_LOCAL_ARCHIVE}")
    FetchContent_Declare(
        nlohmann_json
        URL file://${JSON_LOCAL_ARCHIVE}
    )
else()
    message(STATUS "Local json.tar.xz not found, downloading from GitHub")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    )
endif()

FetchContent_MakeAvailable(nlohmann_json)

add_executable(oper
    src/main.cpp
    src/ConfigLoader.cpp
    src/LogMonitor.cpp
    src/ActionExecutor.cpp
)

target_link_libraries(oper PRIVATE nlohmann_json::nlohmann_json)

install(TARGETS oper DESTINATION /usr/local/bin)
install(DIRECTORY config/ DESTINATION /etc/oper/algorithms)
install(FILES oper.service DESTINATION /etc/systemd/system)
