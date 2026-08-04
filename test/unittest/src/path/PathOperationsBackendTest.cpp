// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathBackendTestBase.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/path/impl/BackendFactory.hpp>
#include <erbsland/path/impl/PathInfoData.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathTempDirectoryOptions.hpp>
#include <erbsland/system/PosixErrorContext.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimePoint.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cerrno>
#include <memory>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathOperations PathTempDirectoryOptions PathBackend)
class PathOperationsBackendTest final : public el::UnitTest {
    class TestBackend final : public PathBackendTestBase {
    public:
        [[nodiscard]] auto currentDirectoryOrThrow() const -> el::path::Path override {
            return el::path::Path{"/work"_el};
        }
        [[nodiscard]] auto resolveOrThrow(
            const el::path::Path &path, [[maybe_unused]] el::path::PathResolveOptions options) const
            -> el::path::Path override {
            return path.isAbsolute() ? path : el::path::Path{"/work"_el} / path;
        }
        [[nodiscard]] auto loadInfoOrThrow(const el::path::Path &path, const el::path::PathInfoParts parts) const
            -> el::path::impl::PathInfoData override {
            auto result = el::path::impl::PathInfoData{};
            result.resolvedPath = resolveOrThrow(path, {});
            result.exists = false;
            result.loadedParts = parts | el::path::PathInfoPart::Type;
            result.lastRefresh = el::time::TimePoint::now();
            return result;
        }
        void createDirectoryEntryOrThrow(
            const el::path::Path &path, [[maybe_unused]] el::path::PathAccessProfile profile) const override {
            ++createCount;
            if (throwRuntimeError) {
                throw el::err::RuntimeError{"injected runtime failure"_el};
            }
            throw el::path::PathError{el::path::PathErrorContext{"injected path failure"_el}
                    .setSourcePath(path.toString())
                    .setPlatformContext(el::system::PosixErrorContext::fromErrorCode(errorCode))};
        }

        mutable int createCount{};
        int errorCode{EACCES};
        bool throwRuntimeError{false};
    };

    class BackendScope final {
    public:
        explicit BackendScope(std::unique_ptr<TestBackend> backend) : backend{backend.get()} {
            el::path::impl::setPathBackend(std::move(backend));
        }
        ~BackendScope() { el::path::impl::setPathBackend(nullptr); }

        TestBackend *backend;
    };

public:
    void testNonThrowingCreationCatchesErbslandExceptions() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};
        scope.backend->throwRuntimeError = true;

        REQUIRE(el::path::Path{"directory"_el}.operations().createDirectory().isFailure());
        REQUIRE_EQUAL(scope.backend->createCount, 1);
    }

    void testTemporaryCreationDoesNotRetryPermissionErrors() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto scope = BackendScope{std::make_unique<TestBackend>()};
        auto options = el::path::PathTempDirectoryOptions{};
        options.setMaximumAttempts(el::unit::ItemCount{10U});

        REQUIRE_THROWS_AS(
            el::path::PathError, el::path::Path{"temporary"_el}.operations().createTempDirectoryOrThrow(options));
        REQUIRE_EQUAL(scope.backend->createCount, 1);
    }

    void testTemporaryCreationRetriesCollisions() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto scope = BackendScope{std::make_unique<TestBackend>()};
        scope.backend->errorCode = EEXIST;
        auto options = el::path::PathTempDirectoryOptions{};
        options.setMaximumAttempts(el::unit::ItemCount{3U});

        const auto temporaryDirectory = el::path::Path{"temporary"_el}.operations().createTempDirectory(options);
        REQUIRE_EQUAL(temporaryDirectory, nullptr);
        REQUIRE_EQUAL(scope.backend->createCount, 3);
    }
};
