// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathBackendTestBase.hpp"
#include "PathTestHelper.hpp"

#include <erbsland/path/impl/BackendFactory.hpp>
#include <erbsland/path/impl/CommonPathBackend.hpp>
#include <erbsland/path/impl/PathInfoData.hpp>
#include <erbsland/path/PathCreateMode.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathResolveMode.hpp>
#include <erbsland/path/PathWriteTextOptions.hpp>
#include <erbsland/stream/ByteOutputStream.hpp>
#include <erbsland/stream/TextOutputStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <optional>
#include <vector>

using el::path::Path;
using el::path::PathResolveMode;
using el::path::PathResolveOptions;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathBackend CommonPathBackend)
class PathResolveBackendTest final : public el::UnitTest {
    class MemoryOutputStream final : public el::stream::ByteOutputStream {
    public: // implement ByteOutputStream
        [[nodiscard]] auto outputSettings() const noexcept -> const el::stream::OutputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::stream::StreamState::Open; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return isReady() ? el::stream::StreamWaitStatus::Ready : el::stream::StreamWaitStatus::Timeout;
        }
        auto flush() -> el::stream::StreamWriteStatus override { return el::stream::StreamWriteStatus::Success; }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

        auto write(std::span<const el::mem::Byte> bytes) -> el::stream::StreamWriteStatus override {
            for (const auto byte : bytes) {
                data.push_back(byte.toUInt8());
            }
            return el::stream::StreamWriteStatus::Success;
        }

    public:
        std::vector<uint8_t> data;

    private:
        el::stream::OutputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

    class TestBackend final : public PathBackendTestBase {
    public:
        [[nodiscard]] auto currentDirectoryOrThrow() const -> Path override {
            if (failCurrentDirectory) {
                throw el::path::PathError{"current directory failed"_el};
            }
            return currentDirectory;
        }

        [[nodiscard]] auto userHomeDirectoryOrThrow() const -> Path override {
            if (failUserHomeDirectory) {
                throw el::path::PathError{"user home directory failed"_el};
            }
            return userHomeDirectory;
        }

        [[nodiscard]] auto resolveOrThrow(const Path &path, const PathResolveOptions options) const -> Path override {
            lastResolvePath = path;
            lastResolveMode = options.mode();
            if (failResolve) {
                throw el::path::PathError{
                    el::path::PathErrorContext{"resolve failed"_el}.setSourcePath(path.toString())};
            }
            if (resolveResult.has_value()) {
                return *resolveResult;
            }
            return absoluteLexicalPathOrThrow(path);
        }

        [[nodiscard]] auto loadInfoOrThrow(const Path &path, el::path::PathInfoParts parts) const
            -> el::path::impl::PathInfoData override {
            auto result = el::path::impl::PathInfoData{};
            result.resolvedPath = resolveOrThrow(path, PathResolveOptions{});
            result.exists = true;
            result.loadedParts = parts | el::path::PathInfoPart::Type;
            return result;
        }

    protected:
        [[nodiscard]] auto openByteOutputStreamWithExistingContentOrThrow(
            const Path &path, const el::path::PathWriteDataOptions options) const
            -> PathByteOutputStreamOpenResult override {
            lastOutputPath = path;
            lastWriteOptions = options;
            lastOutputStream = std::make_shared<MemoryOutputStream>();
            return {lastOutputStream, outputHasExistingContent};
        }

    public:
        Path currentDirectory{"/work/root"_el};
        Path userHomeDirectory{"/home/test-user"_el};
        std::optional<Path> resolveResult;
        mutable Path lastResolvePath;
        mutable PathResolveMode lastResolveMode = PathResolveMode::Physical;
        mutable Path lastOutputPath;
        mutable el::path::PathWriteDataOptions lastWriteOptions;
        mutable std::shared_ptr<MemoryOutputStream> lastOutputStream;
        bool outputHasExistingContent{false};
        bool failCurrentDirectory = false;
        bool failUserHomeDirectory = false;
        bool failResolve = false;
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
    void testResolveDelegatesAndCatchesErrors() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};
        scope.backendPtr->resolveResult = Path{"/mock/result"_el};

        const auto resolved = Path{"draft/path"_el}.resolve(PathResolveMode::Weak);
        REQUIRE_EQUAL(toStdString(resolved), "/mock/result");
        REQUIRE_EQUAL(toStdString(scope.backendPtr->lastResolvePath), "draft/path");
        REQUIRE_EQUAL(scope.backendPtr->lastResolveMode, PathResolveMode::Weak);

        scope.backendPtr->failResolve = true;
        REQUIRE(Path{"draft/path"_el}.resolve().isEmpty());
        REQUIRE_THROWS_AS(el::path::PathError, Path{"draft/path"_el}.resolveOrThrow());
    }

    void testToAbsoluteUsesCurrentDirectoryAndNormalizesLexically() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        REQUIRE_EQUAL(toStdString(Path{"."_el}.toAbsolute()), "/work/root");
        REQUIRE_EQUAL(toStdString(Path{"logs/../data/file.txt"_el}.toAbsolute()), "/work/root/data/file.txt");
        REQUIRE_EQUAL(toStdString(Path{"/already/absolute"_el}.toAbsolute()), "/already/absolute");

        REQUIRE(Path{"data"_el}.toAbsolute(Path{"relative/base"_el}).isEmpty());
        REQUIRE_THROWS_AS(el::path::PathError, Path{"/already/absolute"_el}.toAbsoluteOrThrow(Path{}));
    }

    void testToRelativeValidatesBaseAndReturnsDotForSamePath() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        REQUIRE_EQUAL(
            toStdString(Path{"/work/root/data/file.txt"_el}.toRelative(Path{"/work/root"_el})), "data/file.txt");
        REQUIRE_EQUAL(toStdString(Path{"/work/root"_el}.toRelative(Path{"/work/root"_el})), ".");
        REQUIRE_EQUAL(toStdString(Path{"relative/path"_el}.toRelative(Path{"/work/root"_el})), "relative/path");
        REQUIRE_EQUAL(
            toStdString(Path{"/work/root/file.txt"_el}.toRelative(Path{"/other/root"_el})), "../../work/root/file.txt");

        REQUIRE(Path{"relative/path"_el}.toRelative(Path{"relative/base"_el}).isEmpty());
        REQUIRE_THROWS_AS(el::path::PathError, Path{"relative/path"_el}.toRelativeOrThrow(Path{}));
    }

    void testIsRelativeToAndCommonAncestor() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        REQUIRE(Path{"relative/path"_el}.isRelativeTo(Path{}));
        REQUIRE(Path{"/work/root/data/file.txt"_el}.isRelativeTo(Path{"/work/root"_el}));
        REQUIRE_FALSE(Path{"/work/root/data/file.txt"_el}.isRelativeTo(Path{"/other/root"_el}));
        REQUIRE_EQUAL(
            toStdString(Path{"/work/root/data/file.txt"_el}.commonAncestor(Path{"/work/root/docs/readme.txt"_el})),
            "/work/root");
        REQUIRE_EQUAL(toStdString(Path{"/work/root/data/file.txt"_el}.commonAncestor(Path{"/other/root"_el})), "/");
    }

    void testCurrentDirectoryReportsBackendFailureAsEmptyPath() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        REQUIRE_EQUAL(toStdString(Path::currentDirectory()), "/work/root");
        scope.backendPtr->failCurrentDirectory = true;
        REQUIRE(Path::currentDirectory().isEmpty());
    }

    void testUserHomeDirectoryDelegatesAndReportsBackendFailure() {
        auto scope = BackendScope{std::make_unique<TestBackend>()};

        REQUIRE_EQUAL(toStdString(Path::userHomeDirectory()), "/home/test-user");
        REQUIRE_EQUAL(toStdString(Path::userHomeDirectoryOrThrow()), "/home/test-user");

        scope.backendPtr->failUserHomeDirectory = true;
        REQUIRE(Path::userHomeDirectory().isEmpty());
        REQUIRE_THROWS_AS(el::path::PathError, Path::userHomeDirectoryOrThrow());
    }

    void testTextOutputSuppressesBomForExistingAppendTarget() {
        auto backend = TestBackend{};
        backend.outputHasExistingContent = true;

        auto options = el::path::PathWriteTextOptions{el::text::StringEncoding::Utf16};
        options.setBomMode(el::text::StringBomMode::Require);
        options.setCreationMode(el::path::PathCreateMode::CreateOrAppend);
        const auto stream = backend.openTextOutputStreamOrThrow(Path{"report.txt"_el}, options);

        stream->write("A"_el);

        REQUIRE_EQUAL(backend.lastOutputStream->data, std::vector<uint8_t>({0x41U, 0x00U}));
        REQUIRE_EQUAL(toStdString(backend.lastOutputPath), "report.txt");
        REQUIRE_EQUAL(backend.lastWriteOptions.creationMode(), el::path::PathCreateMode::CreateOrAppend);
    }

    void testTextOutputWritesBomForNewAppendTarget() {
        auto backend = TestBackend{};

        auto options = el::path::PathWriteTextOptions{el::text::StringEncoding::Utf16};
        options.setBomMode(el::text::StringBomMode::Require);
        options.setCreationMode(el::path::PathCreateMode::CreateOrAppend);
        const auto stream = backend.openTextOutputStreamOrThrow(Path{"report.txt"_el}, options);

        stream->write("A"_el);

        REQUIRE_EQUAL(backend.lastOutputStream->data, std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U}));
    }

    void testByteOutputUsesOpenResultHook() {
        auto backend = TestBackend{};
        auto options = el::path::PathWriteDataOptions{};
        options.setCreateParents(true);
        const auto stream = backend.openByteOutputStreamOrThrow(Path{"report.bin"_el}, options);

        REQUIRE(stream == backend.lastOutputStream);
        REQUIRE_EQUAL(toStdString(backend.lastOutputPath), "report.bin");
        REQUIRE(backend.lastWriteOptions.createParents());
    }
};
