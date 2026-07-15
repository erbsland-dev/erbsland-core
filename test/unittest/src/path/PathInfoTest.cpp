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
            if (failResolve) {
                throw el::path::PathError{
                    el::path::PathErrorContext{"resolve failed"_el}.setSourcePath(path.toString())};
            }
            return Path{"/resolved"_el} / path;
        }

        [[nodiscard]] auto loadInfoOrThrow(const Path &path, const PathInfoParts parts) const
            -> el::path::impl::PathInfoData override {
            ++loadCount;
            lastParts = parts;
            if (failLoad) {
                throw el::path::PathError{el::path::PathErrorContext{"info failed"_el}.setSourcePath(path.toString())};
            }
            auto result = el::path::impl::PathInfoData{path};
            result.resolvedPath = resolveOrThrow(path, PathResolveOptions{});
            result.exists = true;
            result.type = PathType::RegularFile;
            result.loadedParts = parts | PathInfoPart::Type;
            result.lastRefresh = el::time::TimePoint::now();
            if (parts.isSet(PathInfoPart::Size)) {
                result.fileSize = el::unit::ByteLength{123U};
            }
            if (parts.isSet(PathInfoPart::Times)) {
                result.lastModified = el::time::DateTime::fromPosixTime(el::time::Seconds{42});
            }
            if (parts.isSet(PathInfoPart::OwnerId)) {
                result.ownerId = el::system::UserId{"42"_el};
            }
            if (parts.isSet(PathInfoPart::AccessRights)) {
                result.accessInfo.setCurrentProcessRights(el::path::PathAccessRights{el::path::PathAccessRight::Read});
            }
            return result;
        }

        mutable int loadCount{};
        mutable PathInfoParts lastParts;
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
        REQUIRE(&info.path() == &Path::empty());
        REQUIRE(&info.resolvedPath() == &Path::empty());
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
