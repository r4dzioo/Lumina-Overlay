#include "Overlay/Hook/PresentHook.h"

namespace overlay::Hook {

bool ExternalOverlayPresentHook::Install() {
    // The production-safe default is an external transparent DX11 overlay window plus
    // read-only telemetry. This intentionally avoids injecting into protected games.
    return true;
}

void ExternalOverlayPresentHook::Uninstall() {
}

#if defined(OVERLAY_ENABLE_IN_PROCESS_HOOKING)
bool OwnedProcessPresentHook::Install() {
    // Extension seam for applications you own and can explicitly load this overlay into.
    // Wire IDXGISwapChain::Present interception here only inside a consented process.
    return false;
}

void OwnedProcessPresentHook::Uninstall() {
}
#endif

} // namespace overlay::Hook

