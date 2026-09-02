// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MacosSystemInfoBackend.hpp"

#include "PosixErrorContext.hpp"
#include "ProcessIdAccess.hpp"

#include "../../path/Path.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringFormat.hpp"
#include "../../time/DateTime.hpp"
#include "../../time/TimeEpoch.hpp"

#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>

namespace erbsland::system::impl {

using namespace text::literals;

auto MacosSystemInfoBackend::currentProcessId() const noexcept -> ProcessId {
    return ProcessIdAccess::fromNative(static_cast<std::uint64_t>(::getpid()));
}

auto MacosSystemInfoBackend::loadProcessInfo(const ProcessId processId) const -> ProcessInfoData {
    if (!processId.isValid()) {
        auto data = ProcessInfoData{};
        data.processError = {"Process does not exist."_el, {}};
        return data;
    }
    return loadNativeProcessInfo(ProcessIdAccess::toNative(processId));
}

auto MacosSystemInfoBackend::operatingSystem() const noexcept -> OperatingSystem {
    return OperatingSystem::Macos;
}

auto MacosSystemInfoBackend::cpuArchitecture() const noexcept -> CpuArchitecture {
    auto arm64 = int{};
    auto arm64Size = sizeof(arm64);
    if (::sysctlbyname("hw.optional.arm64", &arm64, &arm64Size, nullptr, 0) == 0 && arm64 != 0) {
        return CpuArchitecture::Arm64;
    }
    auto machine = std::array<char, 64U>{};
    auto machineSize = machine.size();
    if (::sysctlbyname("hw.machine", machine.data(), &machineSize, nullptr, 0) == 0 && machineSize > 0U) {
        const auto architecture =
            architectureFromName(std::string_view{machine.data(), ::strnlen(machine.data(), machine.size())});
        if (architecture != CpuArchitecture::Unknown) {
            return architecture;
        }
    }
    auto systemName = utsname{};
    return ::uname(&systemName) == 0 ? architectureFromName(systemName.machine) : CpuArchitecture::Unknown;
}

auto MacosSystemInfoBackend::architectureFromName(const std::string_view name) noexcept -> CpuArchitecture {
    const auto value = text::String{name};
    if (value == "arm64"_el || value == "aarch64"_el) {
        return CpuArchitecture::Arm64;
    }
    if (value == "x86_64"_el || value == "amd64"_el) {
        return CpuArchitecture::X86_64;
    }
    if (value == "i386"_el || value == "i686"_el) {
        return CpuArchitecture::X86;
    }
    if (value == "arm"_el || value == "armv7"_el) {
        return CpuArchitecture::Arm32;
    }
    return CpuArchitecture::Unknown;
}

auto MacosSystemInfoBackend::logicalCpuCount() const noexcept -> std::uint32_t {
    auto count = std::uint32_t{};
    auto size = sizeof(count);
    if (::sysctlbyname("hw.logicalcpu", &count, &size, nullptr, 0) == 0 && count > 0U) {
        return count;
    }
    const auto fallback = std::thread::hardware_concurrency();
    return fallback > 0U ? fallback : 1U;
}

auto MacosSystemInfoBackend::error(text::String reason, const int errorCode) -> ProcessInfoError {
    return {std::move(reason), PosixErrorContext::fromErrorCode(errorCode)};
}

auto MacosSystemInfoBackend::loadNativeProcessInfo(const std::uint64_t processId) -> ProcessInfoData {
    const auto nativeProcessId = static_cast<int>(processId);
    for (auto attempt = 0; attempt < 3; ++attempt) {
        auto first = proc_bsdinfo{};
        errno = 0;
        const auto firstSize =
            ::proc_pidinfo(nativeProcessId, PROC_PIDTBSDINFO, 0, &first, static_cast<int>(sizeof(first)));
        if (firstSize != static_cast<int>(sizeof(first))) {
            const auto errorCode = errno == 0 ? ESRCH : errno;
            auto data = ProcessInfoData{};
            data.processError = error("Process does not exist."_el, errorCode);
            if (errorCode != ESRCH && errorCode != ENOENT) {
                data.lookupError = error("Process information cannot be loaded."_el, errorCode);
            }
            return data;
        }

        auto data = ProcessInfoData{};
        data.exists = true;
        data.parentProcessId = ProcessIdAccess::fromNative(first.pbi_ppid);
        data.ownerId = UserId{text::StringFormat{"{}"_el}.build(first.pbi_uid)};
        const auto start = time::DateTime::fromTicks(
            time::Seconds{static_cast<std::int64_t>(first.pbi_start_tvsec)},
            time::Nanoseconds{static_cast<std::int64_t>(first.pbi_start_tvusec * 1'000ULL)},
            time::TimeEpoch::Posix);
        if (start.has_value()) {
            data.startTime = *start;
        } else {
            data.startTimeError = error("Process start time cannot be represented."_el, EOVERFLOW);
        }

        auto pathBuffer = std::array<char, PROC_PIDPATHINFO_MAXSIZE>{};
        errno = 0;
        const auto pathLength = ::proc_pidpath(nativeProcessId, pathBuffer.data(), pathBuffer.size());
        if (pathLength > 0) {
            const auto length = ::strnlen(pathBuffer.data(), static_cast<std::size_t>(pathLength));
            data.executablePath = path::Path::fromPosix(text::String{std::string_view{pathBuffer.data(), length}});
            if (data.executablePath.isEmpty()) {
                data.executablePathError = error("Process executable path is invalid."_el, EINVAL);
            }
        } else {
            data.executablePathError = error("Process executable path is unavailable."_el, errno == 0 ? EACCES : errno);
        }

        auto second = proc_bsdinfo{};
        errno = 0;
        const auto secondSize =
            ::proc_pidinfo(nativeProcessId, PROC_PIDTBSDINFO, 0, &second, static_cast<int>(sizeof(second)));
        if (secondSize != static_cast<int>(sizeof(second))) {
            const auto errorCode = errno == 0 ? ESRCH : errno;
            auto vanished = ProcessInfoData{};
            vanished.processError = error("Process does not exist."_el, errorCode);
            return vanished;
        }
        if (first.pbi_start_tvsec == second.pbi_start_tvsec && first.pbi_start_tvusec == second.pbi_start_tvusec) {
            return data;
        }
    }
    auto data = ProcessInfoData{};
    data.lookupError = error("Process changed while information was loaded."_el, EAGAIN);
    return data;
}

auto createSystemInfoBackend() -> SystemInfoBackendPtr {
    return std::make_unique<MacosSystemInfoBackend>();
}

}
