#pragma once

#include <windows.h>

namespace overlay::Hook {

class IPresentHook {
public:
    virtual ~IPresentHook() = default;
    virtual bool Install() = 0;
    virtual void Uninstall() = 0;
};

class ExternalOverlayPresentHook final : public IPresentHook {
public:
    bool Install() override;
    void Uninstall() override;
};

#if defined(OVERLAY_ENABLE_IN_PROCESS_HOOKING)
class OwnedProcessPresentHook final : public IPresentHook {
public:
    bool Install() override;
    void Uninstall() override;
};
#endif

} // namespace overlay::Hook

