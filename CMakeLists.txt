cmake_minimum_required(VERSION 3.18)
project(Mod LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Mod SHARED src/main.cpp)

if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -w")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -w")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections,--strip-all -s")

    include(FetchContent)

    # 1. fmt
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.2.1
    )
    FetchContent_MakeAvailable(fmt)

    # 2. Dobby (에러 수정 부분)
    # 문제가 되는 Closure Trampoline 기능을 강제로 끕니다. 
    # 일반적인 DobbyHook 기능은 이 옵션 없이도 아주 잘 작동합니다.
    set(DOBBY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(DOBBY_GENERATE_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(DOBBY_ENABLE_CLOSURE_TRAMPOLINE OFF CACHE BOOL "" FORCE) # <- 핵심: 이걸 꺼야 ASM 에러가 안 납니다.

    FetchContent_Declare(
            dobby
            GIT_REPOSITORY https://github.com/jmpews/Dobby.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(dobby)

    # 3. preloader-android (기존 로직 유지)
    FetchContent_Declare(
            preloader_android
            GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
            GIT_TAG main
    )
    FetchContent_GetProperties(preloader_android)
    if(NOT preloader_android_POPULATED)
        FetchContent_Populate(preloader_android)

        file(READ "${preloader_android_SOURCE_DIR}/src/pl/Logger.h" LOGGER_CONTENT)
        string(REPLACE "#include <format>"        "#include <fmt/format.h>"      LOGGER_CONTENT "${LOGGER_CONTENT}")
        string(REPLACE "std::vformat"             "fmt::vformat"                 LOGGER_CONTENT "${LOGGER_CONTENT}")
        string(REPLACE "std::make_format_args"    "fmt::make_format_args"        LOGGER_CONTENT "${LOGGER_CONTENT}")
        file(WRITE "${preloader_android_SOURCE_DIR}/src/pl/Logger.h" "${LOGGER_CONTENT}")

        add_subdirectory(${preloader_android_SOURCE_DIR} ${preloader_android_BINARY_DIR})
    endif()

    target_link_libraries(preloader PUBLIC fmt::fmt)
    include_directories(${CMAKE_SOURCE_DIR}/src)
    
    target_link_libraries(Mod preloader dobby log)

else()
    target_compile_definitions(Mod PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
