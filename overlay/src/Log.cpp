#include "Overlay/Log.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace overlay {

std::mutex Log::mutex_;
std::filesystem::path Log::path_;

void Log::Initialize(const std::filesystem::path& log_path) {
    std::lock_guard lock(mutex_);
    path_ = log_path;
    std::filesystem::create_directories(path_.parent_path());
    std::wofstream stream(path_, std::ios::app);
    stream << L"\n=== Lumina Overlay session started ===\n";
}

void Log::Info(const std::wstring& message) {
    Write(L"INFO", message);
}

void Log::Warn(const std::wstring& message) {
    Write(L"WARN", message);
}

void Log::Error(const std::wstring& message) {
    Write(L"ERROR", message);
}

std::filesystem::path Log::Path() {
    std::lock_guard lock(mutex_);
    return path_;
}

void Log::Write(const wchar_t* level, const std::wstring& message) {
    std::lock_guard lock(mutex_);
    if (path_.empty()) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time{};
    localtime_s(&local_time, &time);

    std::wofstream stream(path_, std::ios::app);
    stream << L'[' << std::put_time(&local_time, L"%Y-%m-%d %H:%M:%S") << L"] "
           << level << L" " << message << L'\n';
}

} // namespace overlay

