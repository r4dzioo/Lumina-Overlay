#include "Overlay/Telemetry/GpuProvider.h"

#include <dxgi1_4.h>
#include <wrl/client.h>

namespace overlay::Telemetry {

bool GpuProvider::Initialize() {
    initialized_ = true;
    return true;
}

void GpuProvider::Shutdown() {
    initialized_ = false;
}

void GpuProvider::Sample(MetricSnapshot& snapshot) {
    if (!initialized_) {
        return;
    }

#if defined(OVERLAY_ENABLE_VENDOR_GPU)
    // Integrate NVAPI and AMD ADL here in production builds where those SDKs are licensed
    // and installed. Keep this provider read-only: no clock, voltage, or fan mutations.
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())))) {
        return;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, adapter.GetAddressOf()))) {
        return;
    }

    DXGI_ADAPTER_DESC1 desc{};
    if (SUCCEEDED(adapter->GetDesc1(&desc))) {
        snapshot.vram_total_mb = static_cast<float>(desc.DedicatedVideoMemory / (1024ull * 1024ull));
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter3;
    if (SUCCEEDED(adapter.As(&adapter3))) {
        DXGI_QUERY_VIDEO_MEMORY_INFO memory_info{};
        if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memory_info))) {
            snapshot.vram_used_mb = static_cast<float>(memory_info.CurrentUsage / (1024ull * 1024ull));
        }
    }
}

} // namespace overlay::Telemetry

