# Widget and Plugin Model

The overlay core is intentionally small: each widget owns presentation only, while telemetry and config are provided as immutable snapshots or mutable layout records.

To add a built-in widget:

1. Implement `IWidget`.
2. Add source to `overlay/CMakeLists.txt`.
3. Register it in `WidgetRegistry::RegisterDefaults`.
4. Add a default layout in `ConfigManager::EnsureDefaultWidgets`.

Plugin DLLs are loaded from `plugins/` and must export:

```cpp
extern "C" __declspec(dllexport)
bool __cdecl RegisterOverlayWidgets(WidgetRegistry* registry);
```

Keep plugin APIs render-thread-safe:

- No blocking I/O in `Render`.
- No telemetry polling in `Render`.
- No heap churn per frame.
- No access to game memory.
- No global hooks from widget DLLs.
