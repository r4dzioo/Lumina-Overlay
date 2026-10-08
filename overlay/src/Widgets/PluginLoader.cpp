#include "Overlay/Widgets/PluginLoader.h"

#include "Overlay/Log.h"

namespace overlay::Widgets {

PluginLoader::~PluginLoader() {
    UnloadAll();
}

void PluginLoader::LoadFromDirectory(const std::filesystem::path& directory, WidgetRegistry& registry) {
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != L".dll") {
            continue;
        }

        HMODULE module = LoadLibraryW(entry.path().c_str());
        if (!module) {
            Log::Warn(L"Failed to load plugin: " + entry.path().wstring());
            continue;
        }

        auto register_widgets = reinterpret_cast<RegisterFn>(GetProcAddress(module, "RegisterOverlayWidgets"));
        if (!register_widgets || !register_widgets(&registry)) {
            Log::Warn(L"Plugin did not register widgets: " + entry.path().wstring());
            FreeLibrary(module);
            continue;
        }

        modules_.push_back(module);
        Log::Info(L"Loaded widget plugin: " + entry.path().wstring());
    }
}

void PluginLoader::UnloadAll() {
    for (HMODULE module : modules_) {
        FreeLibrary(module);
    }
    modules_.clear();
}

} // namespace overlay::Widgets

