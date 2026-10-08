#pragma once

#include <windows.h>

namespace overlay {

class CrashHandler final {
public:
    static void Install();
    static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* exception_info);
};

} // namespace overlay

