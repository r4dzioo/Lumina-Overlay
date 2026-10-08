#pragma once

#include <filesystem>
#include <mutex>
#include <string>

namespace overlay {

class Log final {
public:
    static void Initialize(const std::filesystem::path& log_path);
    static void Info(const std::wstring& message);
    static void Warn(const std::wstring& message);
    static void Error(const std::wstring& message);
    static std::filesystem::path Path();

private:
    static void Write(const wchar_t* level, const std::wstring& message);

    static std::mutex mutex_;
    static std::filesystem::path path_;
};

} // namespace overlay

