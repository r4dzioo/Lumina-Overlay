#include "Overlay/Telemetry/PdhSampler.h"

#include <cmath>

namespace overlay::Telemetry {

PdhSampler::~PdhSampler() {
    Shutdown();
}

bool PdhSampler::Initialize() {
    if (PdhOpenQueryW(nullptr, 0, &query_) != ERROR_SUCCESS) {
        query_ = nullptr;
        return false;
    }

    AddCounter(L"\\Processor(_Total)\\% Processor Time", cpu_total_);
    AddCounter(L"\\Processor Information(_Total)\\Processor Frequency", cpu_clock_);
    AddCounter(L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", disk_read_);
    AddCounter(L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", disk_write_);
    AddCounter(L"\\PhysicalDisk(_Total)\\% Disk Time", disk_active_);
    AddCounter(L"\\Network Interface(*)\\Bytes Received/sec", network_rx_);
    AddCounter(L"\\Network Interface(*)\\Bytes Sent/sec", network_tx_);

    PdhCollectQueryData(query_);
    return true;
}

void PdhSampler::Shutdown() {
    if (query_) {
        PdhCloseQuery(query_);
        query_ = nullptr;
    }
    cpu_total_ = nullptr;
    cpu_clock_ = nullptr;
    disk_read_ = nullptr;
    disk_write_ = nullptr;
    disk_active_ = nullptr;
    network_rx_ = nullptr;
    network_tx_ = nullptr;
}

void PdhSampler::Sample(MetricSnapshot& snapshot) {
    if (!query_) {
        return;
    }

    PdhCollectQueryData(query_);
    snapshot.cpu_usage_percent = ReadCounter(cpu_total_);
    snapshot.cpu_clock_mhz = ReadCounter(cpu_clock_);
    snapshot.disk_read_mbps = ReadCounter(disk_read_) / (1024.0f * 1024.0f);
    snapshot.disk_write_mbps = ReadCounter(disk_write_) / (1024.0f * 1024.0f);
    snapshot.disk_active_percent = ReadCounter(disk_active_);
    snapshot.network_rx_kbps = ReadCounter(network_rx_) / 1024.0f;
    snapshot.network_tx_kbps = ReadCounter(network_tx_) / 1024.0f;
}

bool PdhSampler::AddCounter(const wchar_t* path, PDH_HCOUNTER& counter) {
    if (!query_) {
        return false;
    }
    const PDH_STATUS status = PdhAddEnglishCounterW(query_, path, 0, &counter);
    if (status != ERROR_SUCCESS) {
        counter = nullptr;
        return false;
    }
    return true;
}

float PdhSampler::ReadCounter(PDH_HCOUNTER counter) {
    if (!counter) {
        return 0.0f;
    }

    PDH_FMT_COUNTERVALUE value{};
    const PDH_STATUS status = PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value);
    if (status != ERROR_SUCCESS || value.CStatus != ERROR_SUCCESS || !std::isfinite(value.doubleValue)) {
        return 0.0f;
    }
    return static_cast<float>(value.doubleValue);
}

} // namespace overlay::Telemetry
