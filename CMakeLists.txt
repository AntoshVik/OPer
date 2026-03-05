cmake_minimum_required(VERSION 3.14)
project(OPer)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к локальному архиву (если есть)
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
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
endif()

FetchContent_MakeAvailable(nlohmann_json)

# Подключаем pthread и filesystem
find_package(Threads REQUIRED)

add_executable(oper
    src/main.cpp
    src/ConfigLoader.cpp
    src/LogMonitor.cpp
    src/ActionExecutor.cpp
    src/Settings.cpp
)

target_link_libraries(oper PRIVATE
    nlohmann_json::nlohmann_json
    Threads::Threads
    stdc++fs                  # для GCC < 9 (libstdc++fs)
)

# Для GCC >= 9 stdc++fs не нужен, но лишним не будет
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(oper PRIVATE stdc++fs)
endif()

install(TARGETS oper DESTINATION /usr/local/bin)
install(DIRECTORY config/ DESTINATION /etc/oper)
install(FILES oper.service DESTINATION /etc/systemd/system)
