// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LinuxSystemInfoBackend.hpp"

#include "PosixErrorContext.hpp"
#include "ProcessIdAccess.hpp"

#include "../PlatformErrorCategory.hpp"
#include "../PlatformErrorContext.hpp"

#include "../../err/Exception.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../path/Path.hpp"
#include "../../path/PathContent.hpp"
#include "../../path/PathError.hpp"
#include "../../path/PathReadTextOptions.hpp"
#include "../../text/AsciiCategory.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/IntegerBase.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../text/ReadNumberStatus.hpp"
#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringConverter.hpp"
#include "../../text/StringSide.hpp"
#include "../../time/TimeEpoch.hpp"
#include "../../time/Timestamp.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/CpCount.hpp"

#include <sched.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>

namespace erbsland::system::impl {

using namespace path;
using namespace text;
using namespace unit;
using namespace text::literals;

auto LinuxSystemInfoBackend::currentProcessId() const noexcept -> ProcessId {
    return ProcessIdAccess::fromNative(static_cast<std::uint64_t>(::getpid()));
}

auto LinuxSystemInfoBackend::loadProcessInfo(const ProcessId processId) const -> ProcessInfoData {
    if (!processId.isValid()) {
        auto data = ProcessInfoData{};
        data.processError = {"Process does not exist."_el, {}};
        return data;
    }
    return loadNativeProcessInfo(ProcessIdAccess{processId}.toNative());
}

auto LinuxSystemInfoBackend::operatingSystem() const noexcept -> OperatingSystem {
    return OperatingSystem::Linux;
}

auto LinuxSystemInfoBackend::cpuArchitecture() const noexcept -> CpuArchitecture {
    auto info = utsname{};
    if (::uname(&info) != 0) {
        return CpuArchitecture::Unknown;
    }
    const auto value = StringConverter{std::string_view{info.machine}}.toString();
    if (value == "x86_64"_el || value == "amd64"_el) {
        return CpuArchitecture::X86_64;
    }
    if (value == "i386"_el || value == "i486"_el || value == "i586"_el || value == "i686"_el) {
        return CpuArchitecture::X86;
    }
    if (value == "aarch64"_el || value == "arm64"_el) {
        return CpuArchitecture::Arm64;
    }
    if (value.startsWith("arm"_el)) {
        return CpuArchitecture::Arm32;
    }
    return CpuArchitecture::Unknown;
}

auto LinuxSystemInfoBackend::logicalCpuCount() const noexcept -> std::uint32_t {
    auto cpuSet = cpu_set_t{};
    CPU_ZERO(&cpuSet);
    if (::sched_getaffinity(0, sizeof(cpuSet), &cpuSet) == 0) {
        auto count = std::uint32_t{};
        for (auto index = 0; index < CPU_SETSIZE; ++index) {
            if (CPU_ISSET(index, &cpuSet) != 0) {
                ++count;
            }
        }
        if (count > 0U) {
            return count;
        }
    }
    const auto online = ::sysconf(_SC_NPROCESSORS_ONLN);
    if (online > 0) {
        return static_cast<std::uint32_t>(online);
    }
    const auto fallback = std::thread::hardware_concurrency();
    return fallback > 0U ? fallback : 1U;
}

auto LinuxSystemInfoBackend::error(String reason, const int errorCode) -> ProcessInfoError {
    return {std::move(reason), PosixErrorContext::fromErrorCode(errorCode)};
}

auto LinuxSystemInfoBackend::processFilePath(const std::uint64_t processId, const String &name) -> Path {
    return Path::fromPosix("/proc"_el) / String::fromInteger(processId) / name;
}

auto LinuxSystemInfoBackend::readText(const Path &path, ProcessInfoError &readError) -> std::optional<String> {
    try {
        constexpr auto cMaximumLength = ByteCount{4'000'000U};
        return path.content().readTextOrThrow(PathReadTextOptions{cMaximumLength});
    } catch (const PathError &exception) {
        readError = {exception.reason(), exception.platformContext()};
    } catch (const err::RuntimeError &exception) {
        readError = {exception.reason(), {}};
    }
    return std::nullopt;
}

auto LinuxSystemInfoBackend::resolvePath(const Path &path, ProcessInfoError &resolveError) -> Path {
    try {
        return path.resolveOrThrow();
    } catch (const PathError &exception) {
        resolveError = {exception.reason(), exception.platformContext()};
    } catch (const err::RuntimeError &exception) {
        resolveError = {exception.reason(), {}};
    }
    return {};
}

auto LinuxSystemInfoBackend::isNotFound(const ProcessInfoError &operationError) noexcept -> bool {
    return operationError.context != nullptr && operationError.context->category() == PlatformErrorCategory::NotFound;
}

auto LinuxSystemInfoBackend::parseProcessStat(const String &value) -> std::optional<NativeProcessData> {
    const auto parts = value.splitAtLast(CharSet{U')'});
    if (!parts || !parts->second.startsWith(" "_el)) {
        return std::nullopt;
    }
    const auto fields = parts->second.slice(StringSide::Back, ByteIndex{1U});
    if (fields.isEmpty()) {
        return std::nullopt;
    }
    auto reader = StringCharReader{fields};
    if (!reader.read().isAsciiLetter()) {
        return std::nullopt;
    }

    auto integerOptions = IntegerParseOptions{};
    integerOptions.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpCount::one()).setMaximumDigits(CpCount{20U});
    if (reader.advanceWhile(AsciiCategory::Whitespace).isZero()) {
        return std::nullopt;
    }
    const auto parent = reader.parseInteger(integerOptions);
    if (parent.status != ReadNumberStatus::Success) {
        return std::nullopt;
    }
    for (auto field = 0; field < 17; ++field) {
        if (reader.advanceWhile(AsciiCategory::Whitespace).isZero() ||
            reader.advanceUntil(AsciiCategory::Whitespace).isZero()) {
            return std::nullopt;
        }
    }
    if (reader.advanceWhile(AsciiCategory::Whitespace).isZero()) {
        return std::nullopt;
    }
    const auto startTicks = reader.parseInteger(integerOptions);
    if (startTicks.status != ReadNumberStatus::Success) {
        return std::nullopt;
    }
    return NativeProcessData{parent.value, startTicks.value};
}

auto LinuxSystemInfoBackend::parseEffectiveUserId(const String &value) -> std::optional<std::uint64_t> {
    const auto parts = value.splitAtFirst("\nUid:"_el);
    if (!parts) {
        return std::nullopt;
    }
    auto reader = StringCharReader{parts->second};
    auto integerOptions = IntegerParseOptions{};
    integerOptions.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpCount::one()).setMaximumDigits(CpCount{20U});
    if (reader.advanceWhile(AsciiCategory::Blank).isZero()) {
        return std::nullopt;
    }
    const auto realUserId = reader.parseInteger(integerOptions);
    if (realUserId.status != ReadNumberStatus::Success || reader.advanceWhile(AsciiCategory::Blank).isZero()) {
        return std::nullopt;
    }
    const auto effectiveUserId = reader.parseInteger(integerOptions);
    return effectiveUserId.status == ReadNumberStatus::Success ? std::optional<std::uint64_t>{effectiveUserId.value}
                                                               : std::nullopt;
}

auto LinuxSystemInfoBackend::bootTimeSeconds(ProcessInfoError &operationError) -> std::optional<std::uint64_t> {
    const auto value = readText(Path::fromPosix("/proc/stat"_el), operationError);
    if (!value.has_value()) {
        operationError.reason = "System boot time is unavailable."_el;
        return std::nullopt;
    }
    const auto parts = value->splitAtFirst("\nbtime "_el);
    if (!parts) {
        operationError = error("System boot time data is invalid."_el, EINVAL);
        return std::nullopt;
    }
    auto reader = StringCharReader{parts->second};
    auto integerOptions = IntegerParseOptions{};
    integerOptions.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpCount::one()).setMaximumDigits(CpCount{20U});
    const auto seconds = reader.parseInteger(integerOptions);
    if (seconds.status != ReadNumberStatus::Success) {
        operationError = error("System boot time data is invalid."_el, EINVAL);
        return std::nullopt;
    }
    return seconds.value;
}

auto LinuxSystemInfoBackend::processStartTime(const std::uint64_t startTicks, ProcessInfoError &operationError)
    -> time::Timestamp {
    const auto boot = bootTimeSeconds(operationError);
    if (!boot.has_value()) {
        return {};
    }
    const auto ticksPerSecond = ::sysconf(_SC_CLK_TCK);
    if (ticksPerSecond <= 0) {
        operationError = error("Process start time is unavailable."_el, EINVAL);
        return {};
    }
    const auto ticks = static_cast<std::uint64_t>(ticksPerSecond);
    const auto seconds = *boot + startTicks / ticks;
    const auto nanoseconds = (startTicks % ticks) * 1'000'000'000ULL / ticks;
    const auto result = time::Timestamp::fromTicks(
        time::Seconds{static_cast<std::int64_t>(seconds)},
        time::Nanoseconds{static_cast<std::int64_t>(nanoseconds)},
        time::TimeEpoch::Posix)
                            .value_or(time::Timestamp{});
    if (!result.isValid()) {
        operationError = error("Process start time is unavailable."_el, EINVAL);
    }
    return result;
}

auto LinuxSystemInfoBackend::loadNativeProcessInfo(const std::uint64_t processId) -> ProcessInfoData {
    const auto statPath = processFilePath(processId, "stat"_el);
    for (auto attempt = 0; attempt < 3; ++attempt) {
        auto operationError = ProcessInfoError{};
        const auto firstText = readText(statPath, operationError);
        if (!firstText.has_value()) {
            auto data = ProcessInfoData{};
            data.processError = {"Process does not exist."_el, operationError.context};
            if (!isNotFound(operationError)) {
                data.lookupError = {"Process information cannot be loaded."_el, operationError.context};
            }
            return data;
        }
        const auto first = parseProcessStat(*firstText);
        if (!first.has_value()) {
            auto data = ProcessInfoData{};
            data.lookupError = error("Process status data is invalid."_el, EINVAL);
            return data;
        }

        auto data = ProcessInfoData{};
        data.exists = true;
        data.parentProcessId = ProcessIdAccess::fromNative(first->parentProcessId);
        data.startTime = processStartTime(first->startTicks, data.startTimeError);

        operationError = {};
        data.executablePath = resolvePath(processFilePath(processId, "exe"_el), operationError);
        if (data.executablePath.isEmpty()) {
            data.executablePathError = {"Process executable path is unavailable."_el, operationError.context};
        }

        operationError = {};
        const auto status = readText(processFilePath(processId, "status"_el), operationError);
        if (status.has_value()) {
            const auto userId = parseEffectiveUserId(*status);
            if (userId.has_value()) {
                data.ownerId = UserId{String::fromInteger(*userId)};
            } else {
                data.ownerIdError = error("Process owner identifier is unavailable."_el, EINVAL);
            }
        } else {
            data.ownerIdError = {"Process owner identifier is unavailable."_el, operationError.context};
        }

        operationError = {};
        const auto secondText = readText(statPath, operationError);
        if (!secondText.has_value()) {
            auto data = ProcessInfoData{};
            data.processError = {"Process does not exist."_el, operationError.context};
            if (!isNotFound(operationError)) {
                data.lookupError = {"Process information cannot be loaded."_el, operationError.context};
            }
            return data;
        }
        const auto second = parseProcessStat(*secondText);
        if (!second.has_value()) {
            auto invalid = ProcessInfoData{};
            invalid.lookupError = error("Process status data is invalid."_el, EINVAL);
            return invalid;
        }
        if (first->startTicks == second->startTicks) {
            return data;
        }
    }
    auto data = ProcessInfoData{};
    data.lookupError = error("Process changed while information was loaded."_el, EAGAIN);
    return data;
}

auto createSystemInfoBackend() -> SystemInfoBackendPtr {
    return std::make_unique<LinuxSystemInfoBackend>();
}

}
