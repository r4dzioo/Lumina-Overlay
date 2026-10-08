#include "Overlay/Config/ConfigManager.h"

#include <array>
#include <fstream>
#include <nlohmann/json.hpp>

namespace overlay::Config {

namespace {

nlohmann::json ToJson(const Color& color) {
    return nlohmann::json{{"r", color.r}, {"g", color.g}, {"b", color.b}, {"a", color.a}};
}

Color ReadColor(const nlohmann::json& json, const Color& fallback) {
    if (!json.is_object()) {
        return fallback;
    }
    return Color{
        json.value("r", fallback.r),
        json.value("g", fallback.g),
        json.value("b", fallback.b),
        json.value("a", fallback.a),
    };
}

nlohmann::json ToJson(const WidgetLayout& layout) {
    return nlohmann::json{
        {"id", layout.id},
        {"enabled", layout.enabled},
        {"position", {{"x", layout.position.x}, {"y", layout.position.y}}},
        {"size", {{"x", layout.size.x}, {"y", layout.size.y}}},
        {"opacity", layout.opacity},
    };
}

WidgetLayout ReadLayout(const nlohmann::json& json, const WidgetLayout& fallback) {
    WidgetLayout layout = fallback;
    layout.id = json.value("id", layout.id);
    layout.enabled = json.value("enabled", layout.enabled);
    if (json.contains("position")) {
        layout.position.x = json["position"].value("x", layout.position.x);
        layout.position.y = json["position"].value("y", layout.position.y);
    }
    if (json.contains("size")) {
        layout.size.x = json["size"].value("x", layout.size.x);
        layout.size.y = json["size"].value("y", layout.size.y);
    }
    layout.opacity = json.value("opacity", layout.opacity);
    return layout;
}

} // namespace

bool ConfigManager::Initialize(const std::filesystem::path& app_directory) {
    config_path_ = app_directory / "assets" / "config.json";
    theme_path_ = app_directory / "assets" / "theme.json";
    config_ = DefaultConfig();

    std::filesystem::create_directories(config_path_.parent_path());
    if (!std::filesystem::exists(config_path_) || !std::filesystem::exists(theme_path_)) {
        Save();
    }
    return Load();
}

bool ConfigManager::Load() {
    std::lock_guard lock(mutex_);
    config_ = DefaultConfig();

    try {
        if (std::filesystem::exists(config_path_)) {
            std::ifstream stream(config_path_);
            const auto json = nlohmann::json::parse(stream, nullptr, true, true);

            config_.active_profile = json.value("activeProfile", config_.active_profile);
            if (json.contains("profiles") && json["profiles"].is_array()) {
                config_.profiles.clear();
                for (const auto& item : json["profiles"]) {
                    Profile profile;
                    profile.name = item.value("name", profile.name);
                    profile.global_scale = item.value("globalScale", profile.global_scale);
                    profile.opacity = item.value("opacity", profile.opacity);
                    profile.click_through = item.value("clickThrough", profile.click_through);
                    profile.rgb_accent = item.value("rgbAccent", profile.rgb_accent);

                    if (item.contains("widgets") && item["widgets"].is_array()) {
                        for (const auto& widget_json : item["widgets"]) {
                            WidgetLayout fallback;
                            fallback.id = widget_json.value("id", fallback.id);
                            profile.widgets.push_back(ReadLayout(widget_json, fallback));
                        }
                    }
                    EnsureDefaultWidgets(profile);
                    config_.profiles.push_back(profile);
                }
            }
        }

        if (std::filesystem::exists(theme_path_)) {
            std::ifstream stream(theme_path_);
            const auto json = nlohmann::json::parse(stream, nullptr, true, true);
            config_.theme.name = json.value("name", config_.theme.name);
            config_.theme.background = ReadColor(json.value("background", nlohmann::json{}), config_.theme.background);
            config_.theme.panel = ReadColor(json.value("panel", nlohmann::json{}), config_.theme.panel);
            config_.theme.panel_hover = ReadColor(json.value("panelHover", nlohmann::json{}), config_.theme.panel_hover);
            config_.theme.text = ReadColor(json.value("text", nlohmann::json{}), config_.theme.text);
            config_.theme.muted = ReadColor(json.value("muted", nlohmann::json{}), config_.theme.muted);
            config_.theme.accent = ReadColor(json.value("accent", nlohmann::json{}), config_.theme.accent);
            config_.theme.warning = ReadColor(json.value("warning", nlohmann::json{}), config_.theme.warning);
            config_.theme.danger = ReadColor(json.value("danger", nlohmann::json{}), config_.theme.danger);
        }
    } catch (...) {
        config_ = DefaultConfig();
        return false;
    }

    if (config_.profiles.empty()) {
        config_ = DefaultConfig();
    }
    return true;
}

bool ConfigManager::Save() {
    std::lock_guard lock(mutex_);
    std::filesystem::create_directories(config_path_.parent_path());

    nlohmann::json config_json;
    config_json["activeProfile"] = config_.active_profile;
    config_json["profiles"] = nlohmann::json::array();

    for (const auto& profile : config_.profiles) {
        nlohmann::json profile_json{
            {"name", profile.name},
            {"globalScale", profile.global_scale},
            {"opacity", profile.opacity},
            {"clickThrough", profile.click_through},
            {"rgbAccent", profile.rgb_accent},
            {"widgets", nlohmann::json::array()},
        };
        for (const auto& widget : profile.widgets) {
            profile_json["widgets"].push_back(ToJson(widget));
        }
        config_json["profiles"].push_back(profile_json);
    }

    nlohmann::json theme_json{
        {"name", config_.theme.name},
        {"background", ToJson(config_.theme.background)},
        {"panel", ToJson(config_.theme.panel)},
        {"panelHover", ToJson(config_.theme.panel_hover)},
        {"text", ToJson(config_.theme.text)},
        {"muted", ToJson(config_.theme.muted)},
        {"accent", ToJson(config_.theme.accent)},
        {"warning", ToJson(config_.theme.warning)},
        {"danger", ToJson(config_.theme.danger)},
    };

    std::ofstream config_stream(config_path_);
    std::ofstream theme_stream(theme_path_);
    config_stream << config_json.dump(2);
    theme_stream << theme_json.dump(2);
    return config_stream.good() && theme_stream.good();
}

OverlayConfig ConfigManager::Snapshot() const {
    std::lock_guard lock(mutex_);
    return config_;
}

Profile ConfigManager::ActiveProfile() const {
    std::lock_guard lock(mutex_);
    for (const auto& profile : config_.profiles) {
        if (profile.name == config_.active_profile) {
            return profile;
        }
    }
    return config_.profiles.empty() ? Profile{} : config_.profiles.front();
}

Theme ConfigManager::ActiveTheme() const {
    std::lock_guard lock(mutex_);
    return config_.theme;
}

void ConfigManager::SetClickThrough(bool enabled) {
    std::lock_guard lock(mutex_);
    for (auto& profile : config_.profiles) {
        if (profile.name == config_.active_profile) {
            profile.click_through = enabled;
            return;
        }
    }
}

void ConfigManager::SetGlobalScale(float scale) {
    std::lock_guard lock(mutex_);
    for (auto& profile : config_.profiles) {
        if (profile.name == config_.active_profile) {
            profile.global_scale = scale;
            return;
        }
    }
}

void ConfigManager::UpdateWidgetLayout(const WidgetLayout& layout) {
    std::lock_guard lock(mutex_);
    for (auto& profile : config_.profiles) {
        if (profile.name != config_.active_profile) {
            continue;
        }
        for (auto& widget : profile.widgets) {
            if (widget.id == layout.id) {
                widget = layout;
                return;
            }
        }
        profile.widgets.push_back(layout);
        return;
    }
}

OverlayConfig ConfigManager::DefaultConfig() {
    OverlayConfig config;
    Profile profile;
    profile.name = "Default";
    EnsureDefaultWidgets(profile);
    config.profiles.push_back(profile);
    return config;
}

void ConfigManager::EnsureDefaultWidgets(Profile& profile) {
    const std::array<WidgetLayout, 6> defaults{{
        WidgetLayout{"fps", true, {24.0f, 24.0f}, {248.0f, 112.0f}, 0.82f},
        WidgetLayout{"frametime", true, {24.0f, 148.0f}, {360.0f, 140.0f}, 0.78f},
        WidgetLayout{"cpu", true, {24.0f, 300.0f}, {248.0f, 112.0f}, 0.78f},
        WidgetLayout{"gpu", true, {286.0f, 300.0f}, {248.0f, 112.0f}, 0.78f},
        WidgetLayout{"ram", true, {24.0f, 424.0f}, {248.0f, 96.0f}, 0.78f},
        WidgetLayout{"network", true, {286.0f, 424.0f}, {248.0f, 96.0f}, 0.78f},
    }};

    for (const auto& fallback : defaults) {
        bool exists = false;
        for (const auto& widget : profile.widgets) {
            if (widget.id == fallback.id) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            profile.widgets.push_back(fallback);
        }
    }
}

} // namespace overlay::Config
