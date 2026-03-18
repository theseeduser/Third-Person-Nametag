cmake_minimum_required(VERSION 3.18)
project(Mod LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일 설정
add_library(Mod SHARED src/main.cpp)

if(ANDROID)
    # 최적화 및 가시성 설정
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -w")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -w")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections,--strip-all -s")

    include(FetchContent)

    # 1. fmt 라이브러리 (기존 유지)
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.2.1
    )
    FetchContent_MakeAvailable(fmt)

    # 2. Dobby 라이브러리 추가 (새로 추가됨)
    FetchContent_Declare(
            dobby
            GIT_REPOSITORY https://github.com/jmpews/Dobby.git
            GIT_TAG master
            SOURCE_SUBDIR .
    )
    FetchContent_MakeAvailable(dobby)

    # 3. preloader-android (기존 유지 및 로직 포함)
    FetchContent_Declare(
            preloader_android
            GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
            GIT_TAG main
    )
    FetchContent_GetProperties(preloader_android)
    if(NOT preloader_android_POPULATED)
        FetchContent_Populate(preloader_android)

        # Logger.h에서 std::format을 fmt로 교체하는 기존 로직
        file(READ "${preloader_android_SOURCE_DIR}/src/pl/Logger.h" LOGGER_CONTENT)
        string(REPLACE "#include <format>"        "#include <fmt/format.h>"      LOGGER_CONTENT "${LOGGER_CONTENT}")
        string(REPLACE "std::vformat"             "fmt::vformat"                 LOGGER_CONTENT "${LOGGER_CONTENT}")
        string(REPLACE "std::make_format_args"    "fmt::make_format_args"        LOGGER_CONTENT "${LOGGER_CONTENT}")
        file(WRITE "${preloader_android_SOURCE_DIR}/src/pl/Logger.h" "${LOGGER_CONTENT}")

        add_subdirectory(${preloader_android_SOURCE_DIR} ${preloader_android_BINARY_DIR})
    endif()

    # 라이브러리들 간의 종속성 연결
    target_link_libraries(preloader PUBLIC fmt::fmt)

    # 헤더 경로 설정
    include_directories(${CMAKE_SOURCE_DIR}/src)
    
    # 4. 최종 타겟(Mod)에 Dobby와 preloader, log 연결
    target_link_libraries(Mod 
        preloader 
        dobby      # Dobby 링크 추가
        log        # 안드로이드 로그 라이브러리
    )

else()
    target_compile_definitions(Mod PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
