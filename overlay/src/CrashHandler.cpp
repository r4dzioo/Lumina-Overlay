#include "Overlay/CrashHandler.h"

#include "Overlay/Log.h"

#include <dbghelp.h>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace overlay {

void CrashHandler::Install() {
    SetUnhandledExceptionFilter(&CrashHandler::UnhandledExceptionFilter);
}

LONG WINAPI CrashHandler::UnhandledExceptionFilter(EXCEPTION_POINTERS* exception_info) {
    Log::Error(L"Unhandled exception captured; writing crash dump.");

    const auto log_path = Log::Path();
    const auto dump_dir = log_path.empty() ? std::filesystem::current_path() : log_path.parent_path();
    std::filesystem::create_directories(dump_dir);

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time{};
    localtime_s(&local_time, &time);

    std::wstringstream name;
    name << L"OverlayCrash-" << std::put_time(&local_time, L"%Y%m%d-%H%M%S") << L".dmp";
    const auto dump_path = dump_dir / name.str();

    HANDLE file = CreateFileW(
        dump_path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION info{};
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = exception_info;
        info.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            file,
            MiniDumpWithIndirectlyReferencedMemory,
            &info,
            nullptr,
            nullptr);
        CloseHandle(file);
        Log::Info(L"Crash dump written: " + dump_path.wstring());
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace overlay

