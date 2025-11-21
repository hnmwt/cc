cmake_minimum_required(VERSION 3.15)
project(InspectionApp VERSION 1.0.0 LANGUAGES CXX)

# C++20 標準を使用
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ビルドタイプのデフォルト設定
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# コンパイラオプション
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 出力ディレクトリ設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# インクルードディレクトリ
include_directories(${PROJECT_SOURCE_DIR}/include)

# ===============================
# 依存ライブラリの検索
# ===============================

# OpenCV 4.x
find_package(OpenCV 4 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

# Boost (Asio, System)
# Boost 1.73以降は新しいCMake設定を使用
find_package(Boost 1.70 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Boost version: ${Boost_VERSION}")
    message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
endif()

# Threads (required by Crow and Boost.Asio)
find_package(Threads REQUIRED)

# nlohmann/json (header-only)
# インストール方法: brew install nlohmann-json (macOS) または apt install nlohmann-json3-dev (Ubuntu)
find_package(nlohmann_json 3.9 REQUIRED)
message(STATUS "nlohmann_json found")

# spdlog
# インストール方法: brew install spdlog (macOS) または apt install libspdlog-dev (Ubuntu)
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")

# Crow (header-only) - 手動でインクルードパスを追加
# Crowはヘッダーオンリーライブラリのため、include/crow/*.h に配置する想定
# または、CMakeのFetchContentで取得することも可能
option(USE_CROW_FETCHCONTENT "Fetch Crow library from GitHub" OFF)

if(USE_CROW_FETCHCONTENT)
    include(FetchContent)
    FetchContent_Declare(
        crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.0+5
    )
    FetchContent_MakeAvailable(crow)
    message(STATUS "Crow fetched from GitHub")
else()
    # ローカルにCrowがある場合
    include_directories(${PROJECT_SOURCE_DIR}/external/crow/include)
endif()

# ===============================
# ソースファイルの収集
# ===============================

# ユーティリティソース
file(GLOB_RECURSE UTIL_SOURCES
    "src/utils/*.cpp"
)

# コアソース（フィルタ、検出器、パイプライン、コントローラー）
file(GLOB_RECURSE CORE_SOURCES
    "src/filters/*.cpp"
    "src/detectors/*.cpp"
    "src/pipeline/*.cpp"
    "src/controllers/*.cpp"
)

# I/Oソース
file(GLOB_RECURSE IO_SOURCES
    "src/io/*.cpp"
)

# APIソース
file(GLOB_RECURSE API_SOURCES
    "src/api/*.cpp"
)

# サーバーソース
file(GLOB_RECURSE SERVER_SOURCES
    "src/server/*.cpp"
)

# UIソース（UIアプリケーション専用、メインアプリには含めない）
file(GLOB_RECURSE UI_SOURCES
    "src/ui/*.cpp"
)

# すべてのソースをまとめる（UI除く）
set(ALL_SOURCES
    ${UTIL_SOURCES}
    ${CORE_SOURCES}
    ${IO_SOURCES}
    ${API_SOURCES}
    ${SERVER_SOURCES}
)

# ===============================
# 実行ファイルのビルド
# ===============================

# メインアプリケーション
add_executable(inspection_app
    src/main.cpp
    ${ALL_SOURCES}
)

# リンク対象ライブラリ
target_link_libraries(inspection_app
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# macOSの場合、追加のフレームワークが必要な場合がある
if(APPLE)
    target_link_libraries(inspection_app PRIVATE "-framework CoreFoundation")
endif()

# UIアプリケーション
add_executable(inspection_ui
    src/ui/InspectionUI.cpp
    ${ALL_SOURCES}
)

target_link_libraries(inspection_ui
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(inspection_ui PRIVATE "-framework CoreFoundation")
endif()

# InspectionServer（統合サーバー）
add_executable(inspection_server
    src/server_main.cpp
    ${ALL_SOURCES}
)

target_link_libraries(inspection_server
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(inspection_server PRIVATE "-framework CoreFoundation")
endif()

# テストプログラム: CSVWriter & ImageSaver
add_executable(test_csv_image_saver
    tests/test_csv_image_saver.cpp
    ${ALL_SOURCES}
)

target_link_libraries(test_csv_image_saver
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(test_csv_image_saver PRIVATE "-framework CoreFoundation")
endif()

# テストプログラム: ExternalTriggerHandler
add_executable(test_external_trigger
    tests/test_external_trigger.cpp
    ${ALL_SOURCES}
)

target_link_libraries(test_external_trigger
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(test_external_trigger PRIVATE "-framework CoreFoundation")
endif()

# テストプログラム: RestApiServer
add_executable(test_rest_api
    tests/test_rest_api.cpp
    ${ALL_SOURCES}
)

target_link_libraries(test_rest_api
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(test_rest_api PRIVATE "-framework CoreFoundation")
endif()

# テストプログラム: BlobDetector
add_executable(test_blob_detector
    tests/test_blob_detector.cpp
    ${ALL_SOURCES}
)

target_link_libraries(test_blob_detector
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(test_blob_detector PRIVATE "-framework CoreFoundation")
endif()

# サンプル画像生成プログラム
add_executable(create_blob_samples
    tests/create_blob_samples.cpp
)

target_link_libraries(create_blob_samples
    PRIVATE
        ${OpenCV_LIBS}
)

# テストプログラム: EdgeDetector
add_executable(test_edge_detector
    tests/test_edge_detector.cpp
    ${ALL_SOURCES}
)

target_link_libraries(test_edge_detector
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

if(APPLE)
    target_link_libraries(test_edge_detector PRIVATE "-framework CoreFoundation")
endif()

# ===============================
# インストール設定
# ===============================

install(TARGETS inspection_app
    RUNTIME DESTINATION bin
)

# 設定ファイルのインストール
install(DIRECTORY config/
    DESTINATION etc/inspection_app
    FILES_MATCHING PATTERN "*.json"
)

# ===============================
# テストのビルド（オプション）
# ===============================

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    # Google Test を使用する場合
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

    add_executable(inspection_tests
        ${TEST_SOURCES}
        ${ALL_SOURCES}
    )

    target_link_libraries(inspection_tests
        PRIVATE
            ${OpenCV_LIBS}
            Threads::Threads
            nlohmann_json::nlohmann_json
            spdlog::spdlog
            GTest::GTest
            GTest::Main
    )

    add_test(NAME InspectionTests COMMAND inspection_tests)
endif()

# ===============================
# デバッグ情報の出力
# ===============================

message(STATUS "===========================================")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Source files found: ${ALL_SOURCES}")
message(STATUS "===========================================")
