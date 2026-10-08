cmake_minimum_required(VERSION 3.24)

project(LuminaOverlay VERSION 1.0.0 LANGUAGES CXX)

option(OVERLAY_FETCH_DEPS "Download open-source third-party dependencies with FetchContent" ON)
option(OVERLAY_ENABLE_VENDOR_GPU "Enable NVAPI/AMD ADL provider compilation when SDKs are present" OFF)
option(OVERLAY_ENABLE_IN_PROCESS_HOOKING "Build the owned-process Present hook extension seam. This does not include injection." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "Lumina Overlay targets Windows 10/11 x64 only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Lumina Overlay overlay must be built as x64.")
endif()

include(FetchContent)

if(OVERLAY_FETCH_DEPS)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.9
        GIT_SHALLOW TRUE
    )

    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )

    FetchContent_MakeAvailable(imgui nlohmann_json)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC d3d11 dxgi)
    if(MSVC)
        set_property(TARGET imgui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
else()
    add_subdirectory(third_party/imgui)
    find_package(nlohmann_json CONFIG REQUIRED)
endif()

add_executable(Overlay WIN32
    src/main.cpp
    src/App.cpp
    src/CrashHandler.cpp
    src/Dx11Renderer.cpp
    src/Hotkeys.cpp
    src/Log.cpp
    src/Win32Window.cpp
    src/Config/ConfigManager.cpp
    src/Hook/PresentHook.cpp
    src/Telemetry/GpuProvider.cpp
    src/Telemetry/PdhSampler.cpp
    src/Telemetry/PresentMonClient.cpp
    src/Telemetry/TelemetryService.cpp
    src/Widgets/CpuWidget.cpp
    src/Widgets/FpsWidget.cpp
    src/Widgets/FrametimeWidget.cpp
    src/Widgets/GpuWidget.cpp
    src/Widgets/NetworkWidget.cpp
    src/Widgets/PluginLoader.cpp
    src/Widgets/RamWidget.cpp
    src/Widgets/WidgetRegistry.cpp
)

target_include_directories(Overlay PRIVATE include)

target_compile_definitions(Overlay PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    $<$<BOOL:${OVERLAY_ENABLE_VENDOR_GPU}>:OVERLAY_ENABLE_VENDOR_GPU>
    $<$<BOOL:${OVERLAY_ENABLE_IN_PROCESS_HOOKING}>:OVERLAY_ENABLE_IN_PROCESS_HOOKING>
)

target_link_libraries(Overlay PRIVATE
    imgui
    nlohmann_json::nlohmann_json
    d3d11
    dxgi
    dwmapi
    dbghelp
    pdh
    psapi
    winmm
)

if(MSVC)
    set_property(TARGET Overlay PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(Overlay PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_link_options(Overlay PRIVATE /SUBSYSTEM:WINDOWS)
endif()

add_custom_command(TARGET Overlay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Overlay>/assets
)
