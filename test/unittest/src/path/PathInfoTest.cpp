// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathBackendTestBase.hpp"
#include "PathTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/path/impl/BackendFactory.hpp>
#include <erbsland/path/impl/CommonPathBackend.hpp>
#include <erbsland/path/impl/PathInfoData.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimePoint.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <memory>
#include <thread>

using el::path::Path;
using el::path::PathInfoPart;
using el::path::PathInfoParts;
using el::path::PathResolveOptions;
using el::path::PathType;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(PathInfo Path)
class PathInfoTest final : public el::UnitTest {
    class TestBackend final : public PathBackendTestBase {
    public:
        [[nodiscard]] auto currentDirectoryOrThrow() const -> Path override { return Path{"/work/root"_el}; }

        [[nodiscard]] auto resolveOrThrow(const Path &path, PathResolveOptions) const -> Path override {
            ++resolveCount;
            if (failResolve) {
                throw el::path::PathError{
                    el::path::PathErrorContext{"resolve failed"_el}.setSourcePath(path.toString())};
            }
            return Path{"/resolved"_el} / path;
        }

        [[nodiscard]] auto loadInfoOrThrow(const Path &path, const PathInfoParts parts) const
            -> el::path::impl::PathInfoData override {
            ++fullLoadCount;
            return load(path, resolveOrThrow(path, PathResolveOptions{}), parts);
        }

        [[nodiscard]] auto loadResolvedInfoOrThrow(
            const Path &path, const Path &resolvedPath, const PathInfoParts parts) const
            -> el::path::impl::PathInfoData override {
            ++trustedLoadCount;
            lastTrustedResolvedPath = resolvedPath;
            return load(path, resolvedPath, parts);
        }

    private:
        [[nodiscard]] auto load(const Path &path, const Path &resolvedPath, const PathInfoParts parts) const
            -> el::path::impl::PathInfoData {
            ++loadCount;
            lastParts = parts;
            if (failLoad) {
                throw el::path::PathError{el::path::PathErrorContext{"info failed"_el}.setSourcePath(path.toString())};
            }
            auto result = el::path::impl::PathInfoData{};
            result.resolvedPath = resolvedPath;
            result.exists = true;
            result.type = PathType::RegularFile;
            result.loadedParts = parts | PathInfoPart::Type;
            result.lastRefresh = el::time::TimePoint::now();
            if (parts.isSet(PathInfoPart::Size)) {
                result.fileSize = el::unit::ByteLength{123U};
            }
            if (parts.isSet(PathInfoPart::Times)) {
                result.lastModified =
                    el::time::DateTime::fromTicks(el::time::Seconds{42}, el::time::TimeEpoch::Posix).value();
            }
            if (parts.isSet(PathInfoPart::OwnerId)) {
                result.ownerId = el::system::UserId{"42"_el};
            }
            if (parts.isSet(PathInfoPart::AccessRights)) {
                result.accessInfo.setCurrentProcessRights(el::path::PathAccessRights{el::path::PathAccessRight::Read});
            }
            return result;
        }

    public:
        mutable int loadCount{};
        mutable int fullLoadCount{};
        mutable int trustedLoadCount{};
        mutable int resolveCount{};
        mutable PathInfoParts lastParts;
        mutable Path lastTrustedResolvedPath;
        bool failLoad{false};
        bool failResolve{false};
    };

    class BackendScope final {
    public:
        explicit BackendScope(std::unique_ptr<TestBackend> backend) : backendPtr{backend.get()} {
            el::path::impl::setPathBackend(std::move(backend));
        }
        ~BackendScope() { el::path::impl::setPathBackend(nullptr); }

        TestBackend *backendPtr;
    };

public:
    void testEmptyPathInfoUsesSharedEmptyPath() {
        const auto info = el::path::PathInfo{};

        REQUIRE(info.isEmpty());
        const auto path = &info.path();
        const auto resolvedPath = &info.resolvedPath();
        const auto emptyPath = &Path::empty();
        REQUIRE_EQUAL(path, emptyPath);
        REQUIRE_EQUAL(resolvedPath, emptyPath);
    }

    void testInitialLoadAndPathInfoAccess() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto info = Path{"report.txt"_el}.info();
        REQUIRE_FALSE(info.isEmpty());
        REQUIRE_EQUAL(toStdString(info.path()), "report.txt");
        REQUIRE(info.exists());
        REQUIRE_EQUAL(info.type(), PathType::RegularFile);
        REQUIRE_EQUAL(toStdString(info.resolvedPath()), "/resolved/report.txt");
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);
    }

    void testLazyLoadOfSizePart() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto info = Path{"report.txt"_el}.info();
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);
        REQUIRE_EQUAL(info.fileSize(), el::unit::ByteLength{123U});
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::Size));
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 2);
        REQUIRE_EQUAL(scope.backendPtr->fullLoadCount, 1);
        REQUIRE_EQUAL(scope.backendPtr->trustedLoadCount, 1);
        REQUIRE_EQUAL(scope.backendPtr->resolveCount, 1);
        REQUIRE_EQUAL(toStdString(scope.backendPtr->lastTrustedResolvedPath), "/resolved/report.txt");
    }

    void testPathCopiesShareInformationCache() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto path = Path{"report.txt"_el};
        REQUIRE(path.info().exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);

        REQUIRE(path.info().isRegularFile());
        const auto copiedPath = path;
        REQUIRE(copiedPath.info().exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);
    }

    void testMoveLeavesSourceEmpty() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        auto source = Path{"report.txt"_el}.info();
        auto moved = std::move(source);
        REQUIRE(source.isEmpty());
        REQUIRE(moved.exists());

        auto assigned = el::path::PathInfo{};
        assigned = std::move(moved);
        REQUIRE(moved.isEmpty());
        REQUIRE(assigned.exists());
    }

    void testLazyLoadOfTimeParts() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto info = Path{"report.txt"_el}.info();
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);
        REQUIRE(info.lastModified().isValid());
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::Times));
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 2);
    }

    void testOwnerNameRequestLoadsOwnerId() {
        auto appScope = ApplicationTestScope<>{};
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto info = Path{"report.txt"_el}.info();
        static_cast<void>(info.ownerName());
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::OwnerName));
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::OwnerId));
    }

    void testAccessRightsConvenience() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto info = Path{"report.txt"_el}.info();
        REQUIRE(info.isReadable());
        REQUIRE_FALSE(info.isWritable());
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::AccessRights));
    }

    void testReloadWithExplicitParts() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        auto info = Path{"report.txt"_el}.info();
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);
        info.reload(PathInfoPart::Times);
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::Type));
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::Times));
        REQUIRE_FALSE(scope.backendPtr->lastParts.isSet(PathInfoPart::Size));
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 2);

        REQUIRE_EQUAL(info.fileSize(), el::unit::ByteLength{123U});
        REQUIRE(scope.backendPtr->lastParts.isSet(PathInfoPart::Size));
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 3);
    }

    void testStickyFailureAndReload() {
        auto backend = std::make_unique<TestBackend>();
        backend->failLoad = true;
        auto scope = BackendScope{std::move(backend)};

        auto info = Path{"report.txt"_el}.info();
        REQUIRE_FALSE(info.exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);

        scope.backendPtr->failLoad = false;
        REQUIRE_FALSE(info.exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);

        info.reload();
        REQUIRE(info.exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 2);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCacheExpiresAfterOneSecond() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        const auto info = Path{"report.txt"_el}.info();
        REQUIRE(info.exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 1);

        std::this_thread::sleep_for(std::chrono::milliseconds{1100});
        REQUIRE(info.exists());
        REQUIRE_EQUAL(scope.backendPtr->loadCount, 2);
    }
};
