#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace overlay::Config {

struct Vec2 final {
    float x{0.0f};
    float y{0.0f};
};

struct Color final {
    float r{1.0f};
    float g{1.0f};
    float b{1.0f};
    float a{1.0f};
};

struct WidgetLayout final {
    std::string id;
    bool enabled{true};
    Vec2 position{20.0f, 20.0f};
    Vec2 size{260.0f, 92.0f};
    float opacity{0.78f};
};

struct Profile final {
    std::string name{"Default"};
    float global_scale{1.0f};
    float opacity{0.85f};
    bool click_through{true};
    bool rgb_accent{true};
    std::vector<WidgetLayout> widgets;
};

struct Theme final {
    std::string name{"Midnight Neon"};
    Color background{0.04f, 0.045f, 0.055f, 0.74f};
    Color panel{0.06f, 0.07f, 0.085f, 0.82f};
    Color panel_hover{0.08f, 0.095f, 0.12f, 0.88f};
    Color text{0.92f, 0.95f, 0.98f, 1.0f};
    Color muted{0.55f, 0.62f, 0.70f, 1.0f};
    Color accent{0.05f, 0.82f, 1.0f, 1.0f};
    Color warning{1.0f, 0.72f, 0.20f, 1.0f};
    Color danger{1.0f, 0.25f, 0.32f, 1.0f};
};

struct OverlayConfig final {
    std::string active_profile{"Default"};
    std::vector<Profile> profiles;
    Theme theme;
};

} // namespace overlay::Config

