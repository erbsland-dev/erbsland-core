// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsSystemInfoBackend.hpp"

#include "ProcessIdAccess.hpp"
#include "WindowsErrorContext.hpp"

#include "../../path/Path.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../time/DateTime.hpp"
#include "../../time/TimeEpoch.hpp"

#include <sddl.h>
#include <tlhelp32.h>

#include <bit>
#include <cstdint>
#include <cwchar>
#include <limits>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace erbsland::system::impl {

using namespace text::literals;

auto WindowsSystemInfoBackend::currentProcessId() const noexcept -> ProcessId {
    return ProcessIdAccess::fromNative(static_cast<std::uint64_t>(::GetCurrentProcessId()));
}

auto WindowsSystemInfoBackend::loadProcessInfo(const ProcessId processId) const -> ProcessInfoData {
    const auto rawProcessId = ProcessIdAccess::toNative(processId);
    if (!processId.isValid() || rawProcessId > std::numeric_limits<DWORD>::max()) {
        auto data = ProcessInfoData{};
        data.processError = {"Process does not exist."_el, {}};
        return data;
    }
    const auto nativeProcessId = static_cast<DWORD>(rawProcessId);
    const auto process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, nativeProcessId);
    if (process == nullptr) {
        const auto errorCode = ::GetLastError();
        auto data = ProcessInfoData{};
        data.processError = error("Process does not exist."_el, errorCode);
        if (errorCode == ERROR_ACCESS_DENIED) {
            auto snapshotError = DWORD{};
            if (processEntry(nativeProcessId, snapshotError).has_value()) {
                data.exists = true;
                const auto accessError = error("Process information is inaccessible."_el, errorCode);
                setAttributeErrors(data, accessError);
            } else if (snapshotError != ERROR_SUCCESS) {
                data.lookupError = error("Process information cannot be loaded."_el, snapshotError);
            }
        } else if (errorCode != ERROR_INVALID_PARAMETER && errorCode != ERROR_NOT_FOUND) {
            data.lookupError = error("Process information cannot be loaded."_el, errorCode);
        }
        return data;
    }
    const auto closeProcess = std::unique_ptr<void, decltype(&::CloseHandle)>{process, &::CloseHandle};
    const auto waitResult = ::WaitForSingleObject(process, 0U);
    if (waitResult == WAIT_OBJECT_0) {
        auto data = ProcessInfoData{};
        data.processError = error("Process does not exist."_el, ERROR_NOT_FOUND);
        return data;
    }
    if (waitResult == WAIT_FAILED) {
        auto data = ProcessInfoData{};
        data.lookupError = error("Process state cannot be determined."_el, ::GetLastError());
        return data;
    }
    return loadOpenProcessInfo(nativeProcessId, process);
}

auto WindowsSystemInfoBackend::operatingSystem() const noexcept -> OperatingSystem {
    return OperatingSystem::Windows;
}

auto WindowsSystemInfoBackend::cpuArchitecture() const noexcept -> CpuArchitecture {
    using IsWow64Process2Fn = BOOL(WINAPI *)(HANDLE, USHORT *, USHORT *);
    const auto kernel = ::GetModuleHandleW(L"kernel32.dll");
    if (kernel != nullptr) {
        const auto function = reinterpret_cast<IsWow64Process2Fn>(::GetProcAddress(kernel, "IsWow64Process2"));
        if (function != nullptr) {
            auto processMachine = USHORT{};
            auto nativeMachine = USHORT{};
            if (function(::GetCurrentProcess(), &processMachine, &nativeMachine) != 0) {
                return architectureFromMachine(nativeMachine);
            }
        }
    }
    auto info = SYSTEM_INFO{};
    ::GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return CpuArchitecture::X86_64;
    case PROCESSOR_ARCHITECTURE_INTEL:
        return CpuArchitecture::X86;
    case PROCESSOR_ARCHITECTURE_ARM:
        return CpuArchitecture::Arm32;
    case PROCESSOR_ARCHITECTURE_ARM64:
        return CpuArchitecture::Arm64;
    default:
        return CpuArchitecture::Unknown;
    }
}

auto WindowsSystemInfoBackend::logicalCpuCount() const noexcept -> std::uint32_t {
    auto processMask = DWORD_PTR{};
    auto systemMask = DWORD_PTR{};
    if (::GetProcessAffinityMask(::GetCurrentProcess(), &processMask, &systemMask) != 0 && processMask != 0U) {
        const auto count = std::popcount(processMask);
        if (count > 0) {
            return static_cast<std::uint32_t>(count);
        }
    }
    const auto active = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (active > 0U) {
        return active;
    }
    const auto fallback = std::thread::hardware_concurrency();
    return fallback > 0U ? fallback : 1U;
}

auto WindowsSystemInfoBackend::error(text::String reason, const DWORD errorCode) -> ProcessInfoError {
    return {std::move(reason), WindowsErrorContext::fromErrorCode(errorCode)};
}

void WindowsSystemInfoBackend::setAttributeErrors(ProcessInfoData &data, const ProcessInfoError &queryError) {
    data.executablePathError = queryError;
    data.parentProcessIdError = queryError;
    data.startTimeError = queryError;
    data.ownerIdError = queryError;
}

auto WindowsSystemInfoBackend::processEntry(const DWORD processId, DWORD &errorCode) -> std::optional<PROCESSENTRY32W> {
    const auto snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0U);
    if (snapshot == INVALID_HANDLE_VALUE) {
        errorCode = ::GetLastError();
        return std::nullopt;
    }
    const auto closeSnapshot = std::unique_ptr<void, decltype(&::CloseHandle)>{snapshot, &::CloseHandle};
    auto entry = PROCESSENTRY32W{};
    entry.dwSize = sizeof(entry);
    if (::Process32FirstW(snapshot, &entry) == 0) {
        errorCode = ::GetLastError();
        return std::nullopt;
    }
    do {
        if (entry.th32ProcessID == processId) {
            errorCode = ERROR_SUCCESS;
            return entry;
        }
    } while (::Process32NextW(snapshot, &entry) != 0);
    const auto finalError = ::GetLastError();
    errorCode = finalError == ERROR_NO_MORE_FILES ? ERROR_SUCCESS : finalError;
    return std::nullopt;
}

auto WindowsSystemInfoBackend::architectureFromMachine(const USHORT machine) noexcept -> CpuArchitecture {
    switch (machine) {
    case IMAGE_FILE_MACHINE_AMD64:
        return CpuArchitecture::X86_64;
    case IMAGE_FILE_MACHINE_I386:
        return CpuArchitecture::X86;
    case IMAGE_FILE_MACHINE_ARMNT:
        return CpuArchitecture::Arm32;
    case IMAGE_FILE_MACHINE_ARM64:
        return CpuArchitecture::Arm64;
    default:
        return CpuArchitecture::Unknown;
    }
}

auto WindowsSystemInfoBackend::loadOpenProcessInfo(const DWORD processId, HANDLE process) -> ProcessInfoData {
    auto data = ProcessInfoData{};
    data.exists = true;

    auto creationTime = FILETIME{};
    auto exitTime = FILETIME{};
    auto kernelTime = FILETIME{};
    auto userTime = FILETIME{};
    if (::GetProcessTimes(process, &creationTime, &exitTime, &kernelTime, &userTime) != 0) {
        const auto ticks = (static_cast<std::uint64_t>(creationTime.dwHighDateTime) << 32U) |
            static_cast<std::uint64_t>(creationTime.dwLowDateTime);
        data.startTime = time::DateTime::fromTicks(
            time::Seconds{static_cast<std::int64_t>(ticks / 10'000'000ULL)},
            time::Nanoseconds{static_cast<std::int64_t>((ticks % 10'000'000ULL) * 100ULL)},
            time::TimeEpoch::Windows)
                             .value_or(time::DateTime{});
        if (!data.startTime.isValid()) {
            data.startTimeError = error("Process start time cannot be represented."_el, ERROR_ARITHMETIC_OVERFLOW);
        }
    } else {
        data.startTimeError = error("Process start time is unavailable."_el, ::GetLastError());
    }

    auto pathBuffer = std::vector<wchar_t>(1024U);
    while (pathBuffer.size() <= 32'768U) {
        auto length = static_cast<DWORD>(pathBuffer.size());
        if (::QueryFullProcessImageNameW(process, 0U, pathBuffer.data(), &length) != 0) {
            data.executablePath =
                path::Path::fromWindows(text::StringConverter{std::wstring_view{pathBuffer.data(), length}}.toString());
            if (data.executablePath.isEmpty()) {
                data.executablePathError = error("Process executable path is invalid."_el, ERROR_INVALID_DATA);
            }
            break;
        }
        const auto errorCode = ::GetLastError();
        if (errorCode != ERROR_INSUFFICIENT_BUFFER) {
            data.executablePathError = error("Process executable path is unavailable."_el, errorCode);
            break;
        }
        pathBuffer.resize(pathBuffer.size() * 2U);
    }
    if (data.executablePath.isEmpty() && !data.executablePathError.hasError()) {
        data.executablePathError = error("Process executable path is unavailable."_el, ERROR_INSUFFICIENT_BUFFER);
    }

    auto snapshotError = DWORD{};
    if (const auto entry = processEntry(processId, snapshotError); entry.has_value()) {
        data.parentProcessId = ProcessIdAccess::fromNative(entry->th32ParentProcessID);
    } else if (snapshotError != ERROR_SUCCESS) {
        data.parentProcessIdError = error("Parent process identifier is unavailable."_el, snapshotError);
    } else {
        data.parentProcessIdError = error("Parent process identifier is unavailable."_el, ERROR_NOT_FOUND);
    }

    auto token = HANDLE{};
    if (::OpenProcessToken(process, TOKEN_QUERY, &token) != 0) {
        const auto closeToken = std::unique_ptr<void, decltype(&::CloseHandle)>{token, &::CloseHandle};
        auto tokenLength = DWORD{};
        ::GetTokenInformation(token, TokenUser, nullptr, 0U, &tokenLength);
        const auto sizeError = ::GetLastError();
        if (tokenLength > 0U && sizeError == ERROR_INSUFFICIENT_BUFFER) {
            auto tokenBuffer = std::vector<std::byte>(tokenLength);
            if (::GetTokenInformation(token, TokenUser, tokenBuffer.data(), tokenLength, &tokenLength) != 0) {
                const auto *tokenUser = reinterpret_cast<const TOKEN_USER *>(tokenBuffer.data());
                auto *sidText = static_cast<wchar_t *>(nullptr);
                if (::ConvertSidToStringSidW(tokenUser->User.Sid, &sidText) != 0) {
                    const auto freeSidText = std::unique_ptr<void, decltype(&::LocalFree)>{sidText, &::LocalFree};
                    data.ownerId =
                        UserId{text::StringConverter{std::wstring_view{sidText, std::wcslen(sidText)}}.toString()};
                } else {
                    data.ownerIdError = error("Process owner identifier is unavailable."_el, ::GetLastError());
                }
            } else {
                data.ownerIdError = error("Process owner identifier is unavailable."_el, ::GetLastError());
            }
        } else {
            data.ownerIdError = error("Process owner identifier is unavailable."_el, sizeError);
        }
    } else {
        data.ownerIdError = error("Process owner identifier is unavailable."_el, ::GetLastError());
    }

    const auto finalState = ::WaitForSingleObject(process, 0U);
    if (finalState == WAIT_OBJECT_0) {
        auto vanished = ProcessInfoData{};
        vanished.processError = error("Process does not exist."_el, ERROR_NOT_FOUND);
        return vanished;
    }
    if (finalState == WAIT_FAILED) {
        auto failed = ProcessInfoData{};
        failed.lookupError = error("Process state cannot be determined."_el, ::GetLastError());
        return failed;
    }
    return data;
}

auto createSystemInfoBackend() -> SystemInfoBackendPtr {
    return std::make_unique<WindowsSystemInfoBackend>();
}

}
