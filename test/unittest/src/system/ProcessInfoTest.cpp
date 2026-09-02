// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/system/impl/ProcessIdAccess.hpp>
#include <erbsland/system/impl/SystemInfoBackend.hpp>
#include <erbsland/system/PlatformError.hpp>
#include <erbsland/system/ProcessInfo.hpp>
#include <erbsland/system/SystemInfo.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimeEpoch.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ProcessId ProcessInfo)
class ProcessInfoTest final : public el::UnitTest {
private:
    class TestBackend final : public el::system::impl::SystemInfoBackend {
    public:
        [[nodiscard]] auto currentProcessId() const noexcept -> el::system::ProcessId override {
            return el::system::impl::ProcessIdAccess::fromNative(42U);
        }

        [[nodiscard]] auto loadProcessInfo(el::system::ProcessId) const -> el::system::impl::ProcessInfoData override {
            ++loadCount;
            if (snapshots.empty()) {
                return {};
            }
            const auto index = nextSnapshot < snapshots.size() ? nextSnapshot++ : snapshots.size() - 1U;
            return snapshots[index];
        }

        [[nodiscard]] auto operatingSystem() const noexcept -> el::system::OperatingSystem override {
            return el::system::OperatingSystem::Linux;
        }

        [[nodiscard]] auto cpuArchitecture() const noexcept -> el::system::CpuArchitecture override {
            return el::system::CpuArchitecture::Arm64;
        }

        [[nodiscard]] auto logicalCpuCount() const noexcept -> std::uint32_t override { return logicalCpuCountValue; }

        mutable std::size_t loadCount{};
        mutable std::size_t nextSnapshot{};
        std::vector<el::system::impl::ProcessInfoData> snapshots;
        std::uint32_t logicalCpuCountValue{7U};
    };

    class BackendScope final {
    public:
        explicit BackendScope(std::unique_ptr<TestBackend> backend) : backend{backend.get()} {
            el::system::impl::setSystemInfoBackend(std::move(backend));
        }

        ~BackendScope() { el::system::impl::setSystemInfoBackend(nullptr); }

        TestBackend *backend;
    };

    [[nodiscard]] static auto processId(const std::uint64_t value) -> el::system::ProcessId {
        return el::system::impl::ProcessIdAccess::fromNative(value);
    }

    [[nodiscard]] static auto startTime(const std::int64_t seconds) -> el::time::DateTime {
        return el::time::DateTime::fromSeconds(el::time::Seconds{seconds}, el::time::TimeEpoch::Posix).value();
    }

    [[nodiscard]] static auto snapshot(const std::uint64_t parent, const std::int64_t started)
        -> el::system::impl::ProcessInfoData {
        auto result = el::system::impl::ProcessInfoData{};
        result.exists = true;
        result.executablePath = el::path::Path{"/test/process"_el};
        result.parentProcessId = processId(parent);
        result.startTime = startTime(started);
        result.ownerId = el::system::UserId{"501"_el};
        return result;
    }

public:
    void testProcessIdValue() {
        const auto invalid = el::system::ProcessId{};
        const auto first = processId(41U);
        const auto second = processId(42U);
        REQUIRE_FALSE(invalid.isValid());
        REQUIRE(invalid.toString().isEmpty());
        REQUIRE(first.isValid());
        REQUIRE_EQUAL(first.toString(), "41"_el);
        REQUIRE(first < second);
        REQUIRE(first == processId(41U));
        REQUIRE_EQUAL(first.toHash(), processId(41U).toHash());
        REQUIRE_EQUAL(std::hash<el::system::ProcessId>{}(first), first.toHash());

        auto copy = first;
        auto moved = std::move(copy);
        REQUIRE(moved == first);
        moved = second;
        REQUIRE(moved == second);
    }

    void testEagerStableSnapshotAndReload() {
        auto backend = std::make_unique<TestBackend>();
        backend->snapshots = {snapshot(10U, 100U), snapshot(11U, 200U)};
        const auto scope = BackendScope{std::move(backend)};

        auto info = el::system::ProcessInfo{};
        REQUIRE(info.processId() == processId(42U));
        REQUIRE(info.exists());
        REQUIRE(info.parentProcessId() == processId(10U));
        REQUIRE(info.startTime() == startTime(100U));
        REQUIRE_EQUAL(scope.backend->loadCount, 1U);

        static_cast<void>(info.executablePath());
        static_cast<void>(info.ownerId());
        REQUIRE_EQUAL(scope.backend->loadCount, 1U);

        info.reload();
        REQUIRE(info.parentProcessId() == processId(11U));
        REQUIRE(info.startTime() == startTime(200U));
        REQUIRE_EQUAL(scope.backend->loadCount, 2U);
    }

    void testCopiesKeepIndependentSnapshots() {
        auto backend = std::make_unique<TestBackend>();
        backend->snapshots = {snapshot(10U, 100U), snapshot(11U, 200U)};
        const auto scope = BackendScope{std::move(backend)};

        auto original = el::system::ProcessInfo{processId(50U)};
        auto copy = original;
        original.reload();
        REQUIRE(original.startTime() == startTime(200U));
        REQUIRE(copy.startTime() == startTime(100U));
    }

    void testUnavailableAttributesAndThrowingVariants() {
        auto unavailable = el::system::impl::ProcessInfoData{};
        unavailable.exists = true;
        unavailable.executablePathError.reason = "No executable path."_el;
        unavailable.parentProcessIdError.reason = "No parent process."_el;
        unavailable.startTimeError.reason = "No start time."_el;
        unavailable.ownerIdError.reason = "No owner."_el;
        auto backend = std::make_unique<TestBackend>();
        backend->snapshots = {unavailable};
        const auto scope = BackendScope{std::move(backend)};

        const auto info = el::system::ProcessInfo{processId(50U)};
        REQUIRE(info.exists());
        REQUIRE(info.executablePath().isEmpty());
        REQUIRE_FALSE(info.parentProcessId().isValid());
        REQUIRE_FALSE(info.startTime().isValid());
        REQUIRE(info.ownerId().isEmpty());
        REQUIRE_THROWS_AS(el::system::PlatformError, static_cast<void>(info.executablePathOrThrow()));
        REQUIRE_THROWS_AS(el::system::PlatformError, static_cast<void>(info.parentProcessIdOrThrow()));
        REQUIRE_THROWS_AS(el::system::PlatformError, static_cast<void>(info.startTimeOrThrow()));
        REQUIRE_THROWS_AS(el::system::PlatformError, static_cast<void>(info.ownerIdOrThrow()));
        REQUIRE_THROWS_AS(el::path::PathError, static_cast<void>(el::path::Path::executablePathOrThrow()));
    }

    void testReloadLookupFailure() {
        auto failed = el::system::impl::ProcessInfoData{};
        failed.lookupError.reason = "Lookup failed."_el;
        auto backend = std::make_unique<TestBackend>();
        backend->snapshots = {snapshot(10U, 100U), failed};
        const auto scope = BackendScope{std::move(backend)};

        auto info = el::system::ProcessInfo{processId(50U)};
        REQUIRE_THROWS_AS(el::system::PlatformError, info.reloadOrThrow());
        REQUIRE_FALSE(info.exists());
    }

    void testInvalidIdentifier() {
        auto backend = std::make_unique<TestBackend>();
        backend->snapshots = {el::system::impl::ProcessInfoData{}};
        const auto scope = BackendScope{std::move(backend)};

        const auto info = el::system::ProcessInfo{el::system::ProcessId{}};
        REQUIRE_FALSE(info.processId().isValid());
        REQUIRE_FALSE(info.exists());
    }

    void testLogicalCpuCountFallback() {
        auto backend = std::make_unique<TestBackend>();
        backend->logicalCpuCountValue = 0U;
        const auto scope = BackendScope{std::move(backend)};

        REQUIRE_EQUAL(el::system::info::logicalCpuCount(), 1U);
    }

    void testNativeCurrentProcessAndPathShortcut() {
        el::system::impl::setSystemInfoBackend(nullptr);
        auto info = el::system::ProcessInfo{};
        REQUIRE(info.processId().isValid());
        REQUIRE(info.exists());
        REQUIRE_FALSE(info.executablePath().isEmpty());
        REQUIRE(info.executablePath().isAbsolute());
        REQUIRE(info.parentProcessId().isValid());
        REQUIRE(info.startTime().isValid());
        REQUIRE_FALSE(info.ownerId().isEmpty());
        REQUIRE_EQUAL(el::path::Path::executablePath(), info.executablePath());
        REQUIRE_EQUAL(el::path::Path::executablePathOrThrow(), info.executablePath());

        const auto startTime = info.startTime();
        const auto executablePath = info.executablePath();
        info.reload();
        REQUIRE(info.exists());
        REQUIRE_EQUAL(info.startTime(), startTime);
        REQUIRE_EQUAL(info.executablePath(), executablePath);
    }
};
