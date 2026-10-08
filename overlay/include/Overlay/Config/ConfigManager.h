#pragma once

#include "Overlay/Config/OverlayConfig.h"

#include <filesystem>
#include <mutex>
#include <optional>

namespace overlay::Config {

class ConfigManager final {
public:
    bool Initialize(const std::filesystem::path& app_directory);
    bool Load();
    bool Save();

    OverlayConfig Snapshot() const;
    Profile ActiveProfile() const;
    Theme ActiveTheme() const;

    void SetClickThrough(bool enabled);
    void SetGlobalScale(float scale);
    void UpdateWidgetLayout(const WidgetLayout& layout);

    std::filesystem::path ConfigPath() const { return config_path_; }
    std::filesystem::path ThemePath() const { return theme_path_; }

private:
    static OverlayConfig DefaultConfig();
    static void EnsureDefaultWidgets(Profile& profile);

    mutable std::mutex mutex_;
    OverlayConfig config_;
    std::filesystem::path config_path_;
    std::filesystem::path theme_path_;
};

} // namespace overlay::Config

