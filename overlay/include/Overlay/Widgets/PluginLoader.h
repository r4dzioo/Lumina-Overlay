#pragma once

#include "Overlay/Widgets/WidgetRegistry.h"

#include <filesystem>
#include <vector>
#include <windows.h>

namespace overlay::Widgets {

class PluginLoader final {
public:
    using RegisterFn = bool(__cdecl*)(WidgetRegistry*);

    PluginLoader() = default;
    ~PluginLoader();

    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;

    void LoadFromDirectory(const std::filesystem::path& directory, WidgetRegistry& registry);
    void UnloadAll();

private:
    std::vector<HMODULE> modules_;
};

} // namespace overlay::Widgets

