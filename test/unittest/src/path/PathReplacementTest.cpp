// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"
#include "ReplacementOutputStream.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteInputStream.hpp>
#include <erbsland/DiagnosticHelper.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/path/impl/FileCommit.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/TempByteOutputStream.hpp>
#include <erbsland/TempTextOutputStream.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#if defined(_WIN32)
#include <erbsland/system/impl/WindowsErrorContext.hpp>
#else
#include <erbsland/system/impl/PosixErrorContext.hpp>

#include <cerrno>
#endif
#include <filesystem>
#include <memory>
#include <new>
#include <vector>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(
    PathContent PathCommitOptions PathReplaceDataOptions PathReplaceTextOptions TempByteOutputStream
        TempTextOutputStream)
class PathReplacementTest final : public el::UnitTest {
public:
    void testTextDataAndEncoding() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-content"};
        const auto path = fixture.child("file.txt");
        path.content().replaceTextOrThrow("one"_el);
        REQUIRE(path.content().replaceText("two"_el).isSuccessful());
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "two"_el);
        path.content().replaceTextOrThrow(
            "A"_el, el::path::PathReplaceTextOptions{}.setEncoding(el::text::StringEncoding::Utf16));
        REQUIRE_EQUAL(
            path.content().readDataOrThrow().toUInt8Vector(), std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0U}));
        REQUIRE(path.content().replaceData(el::mem::ByteBlock{}).isSuccessful());
        REQUIRE(path.content().readDataOrThrow().isEmpty());
        const auto nested = fixture.child("a/b/file");
        REQUIRE(nested.content().replaceText("text"_el).isFailure());
        nested.content().replaceTextOrThrow(
            "text"_el,
            el::path::PathReplaceTextOptions{}.setCreateParents(true).setAccessProfile(
                el::path::PathAccessProfile::UserOnly));
        REQUIRE_EQUAL(nested.content().readTextOrThrow(), "text"_el);
    }

    void testBoundedTransfers() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-bounded"};
        const auto path = fixture.child("file");
        const auto settings = el::stream::OutputStreamSettings{}.setBackBufferLimit(el::unit::ByteCount{16U});
        const auto text =
            "A long replacement containing Unicode: Grüße 世界. A long replacement containing Unicode."_el;
        const auto options =
            el::path::PathReplaceTextOptions{}.setStreamSettings(settings).setBackupCount(el::unit::ItemCount{1U});
        path.content().replaceTextOrThrow(text, options);
        path.content().replaceTextOrThrow("new"_el, options);
        REQUIRE_EQUAL(fixture.child("file.bak.1").content().readTextOrThrow(), text);
        const auto data = fixture.child("file.bak.1").content().readDataOrThrow();
        path.content().replaceDataOrThrow(data, el::path::PathReplaceDataOptions{}.setStreamSettings(settings));
        REQUIRE_EQUAL(path.content().readDataOrThrow(), data);
    }

    void testNativeCauseAndBackupSymlinkRejection() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-native-cause"};
        const auto destination = fixture.child("file");
        destination.content().replaceTextOrThrow("old"_el);
        auto temporary = fixture.path().operations().openTempTextOutputStreamOrThrow();
        const auto missing = fixture.child("missing");
        try {
            temporary->commitToOrThrow(
                destination,
                el::path::PathCommitOptions{}.setBeforeCommit(
                    [&missing](const el::path::Path &, const el::path::Path &) -> void {
                        [[maybe_unused]] const auto input = missing.content().openByteInputStream();
                    }));
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            const auto rendered = el::err::DiagnosticHelper{error}.toDocument().toString();
            REQUIRE(rendered.contains("pre-commit check"_el));
            REQUIRE(rendered.contains("Platform Error"_el));
            WITH_CONTEXT(requireNotFoundCause(error));
            REQUIRE(rendered.contains(missing.toString()));
            REQUIRE(error.hasCause());
        }
        try {
            el::path::impl::FileCommit{missing, destination, {}, temporary->outputSettings()}.prepare();
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            const auto rendered = el::err::DiagnosticHelper{error}.toDocument().toString();
            REQUIRE(rendered.contains("Validating the replacement"_el));
            REQUIRE(rendered.contains("Platform Error"_el));
            WITH_CONTEXT(requireNotFoundCause(error));
            REQUIRE(rendered.contains(missing.toString()));
        }
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
        REQUIRE(temporary->removeOnClose());
        temporary->close();
#if !defined(_WIN32)
        std::filesystem::create_symlink(destination.toStdPath(), fixture.child("file.bak.1").toStdPath());
        REQUIRE(destination.content()
                .replaceText("new"_el, el::path::PathReplaceTextOptions{}.setBackupCount(el::unit::ItemCount{1U}))
                .isFailure());
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
        REQUIRE(fixture.child("file.bak.1").info().isSymlink());
        const auto link = fixture.child("link");
        std::filesystem::create_symlink(destination.toStdPath(), link.toStdPath());
        REQUIRE(link.content().replaceText("bad"_el).isFailure());
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
#endif
    }

    void testRetentionAndReducedCount() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-backups"};
        const auto path = fixture.child("file.ext");
        const auto options = el::path::PathReplaceTextOptions{}.setBackupCount(el::unit::ItemCount{3U});
        for (const auto &text : {"one"_el, "two"_el, "three"_el, "four"_el, "five"_el}) {
            path.content().replaceTextOrThrow(text, options);
        }
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "five"_el);
        REQUIRE_EQUAL(fixture.child("file.ext.bak.1").content().readTextOrThrow(), "four"_el);
        REQUIRE_EQUAL(fixture.child("file.ext.bak.2").content().readTextOrThrow(), "three"_el);
        REQUIRE_EQUAL(fixture.child("file.ext.bak.3").content().readTextOrThrow(), "two"_el);
        REQUIRE_FALSE(fixture.child("file.ext.bak.4").info().exists());
        fixture.child("file.ext.bak.01").content().writeTextOrThrow("unmanaged"_el);
        path.content().replaceTextOrThrow(
            "six"_el, el::path::PathReplaceTextOptions{}.setBackupCount(el::unit::ItemCount{1U}));
        REQUIRE_EQUAL(fixture.child("file.ext.bak.1").content().readTextOrThrow(), "five"_el);
        REQUIRE_FALSE(fixture.child("file.ext.bak.2").info().exists());
        REQUIRE_FALSE(fixture.child("file.ext.bak.3").info().exists());
        REQUIRE_EQUAL(fixture.child("file.ext.bak.01").content().readTextOrThrow(), "unmanaged"_el);
        path.content().replaceTextOrThrow("seven"_el);
        REQUIRE_EQUAL(fixture.child("file.ext.bak.1").content().readTextOrThrow(), "five"_el);
    }

    void testBackupPreservesNativeFilename() {
#ifndef ERBSLAND_OS_WINDOWS
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-native-name"};
        const auto path = fixture.path().joinedNative(R"(a\b.txt)"_el);
        path.content().replaceTextOrThrow("old"_el);
        path.content().replaceTextOrThrow(
            "new"_el, el::path::PathReplaceTextOptions{}.setBackupCount(el::unit::ItemCount{1U}));
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "new"_el);
        REQUIRE_EQUAL(path.withName(R"(a\b.txt.bak.1)"_el).content().readTextOrThrow(), "old"_el);
#endif
    }

    void testBackupGapsAndMissingDestination() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-gaps"};
        const auto path = fixture.child("file");
        const auto options = el::path::PathReplaceTextOptions{}.setBackupCount(el::unit::ItemCount{4U});
        path.content().replaceTextOrThrow("old"_el, options);
        REQUIRE_FALSE(fixture.child("file.bak.1").info().exists());
        fixture.child("file.bak.3").content().writeTextOrThrow("older"_el);
        path.content().replaceTextOrThrow("new"_el, options);
        REQUIRE_EQUAL(fixture.child("file.bak.1").content().readTextOrThrow(), "old"_el);
        REQUIRE_FALSE(fixture.child("file.bak.2").info().exists());
        REQUIRE_EQUAL(fixture.child("file.bak.4").content().readTextOrThrow(), "older"_el);
    }

    void testBackupFailurePolicies() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-backup-failure"};
        const auto path = fixture.child("file");
        path.content().replaceTextOrThrow("old"_el);
        fixture.child("file.bak.1").operations().createDirectoryOrThrow();
        auto options = el::path::PathReplaceTextOptions{}.setBackupCount(el::unit::ItemCount{2U});
        REQUIRE_THROWS_AS(el::path::PathError, path.content().replaceTextOrThrow("new"_el, options));
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "old"_el);
        path.content().replaceTextOrThrow("new"_el, options.setIgnoreBackupErrors(true));
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "new"_el);
        REQUIRE(fixture.child("file.bak.1").info().isDirectory());
        REQUIRE_THROWS_AS(
            el::path::PathError,
            path.content().replaceTextOrThrow("bad"_el, options.setCollisionMode(el::path::PathCollisionMode::Skip)));
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "new"_el);
    }

    void testCommitOwnershipChecksAndRetry() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-commit"};
        const auto path = fixture.child("file");
        path.content().replaceTextOrThrow("old"_el);
        auto temporary = fixture.path().operations().openTempTextOutputStreamOrThrow();
        REQUIRE(temporary->write("new"_el).isSuccess());
        const auto source = temporary->path();
        int checks = 0;
        auto options = el::path::PathCommitOptions{}
                           .setBackupCount(el::unit::ItemCount{1U})
                           .setBeforeCommit([&checks](const el::path::Path &, const el::path::Path &) -> void {
                               ++checks;
                               if (checks == 2) {
                                   throw el::err::RuntimeError{"Policy rejected the final move"_el};
                               }
                           });
        REQUIRE_THROWS_AS(el::path::PathError, temporary->commitToOrThrow(path, options));
        REQUIRE_EQUAL(checks, 2);
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "old"_el);
        REQUIRE_EQUAL(fixture.child("file.bak.1").content().readTextOrThrow(), "old"_el);
        REQUIRE_EQUAL(temporary->path(), source);
        REQUIRE(temporary->removeOnClose());
        REQUIRE(temporary->commitTo(path).isSuccessful());
        REQUIRE(temporary->isEmpty());
        REQUIRE_FALSE(source.info().exists());
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "new"_el);
        REQUIRE(temporary->commitTo(path).isFailure());
    }

    void testInvalidDestinationsAndPolicies() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-invalid"};
        REQUIRE(fixture.path().content().replaceText("x"_el).isFailure());
        auto temporary = fixture.path().operations().openTempByteOutputStreamOrThrow();
        REQUIRE(temporary->commitTo(temporary->path()).isFailure());
        REQUIRE(temporary->commitTo(el::path::Path{}).isFailure());
        REQUIRE(temporary
                ->commitTo(
                    fixture.child("new"), el::path::PathCommitOptions{}.setBackupCount(el::unit::ItemCount::infinite()))
                .isFailure());
        const auto path = fixture.child("file");
        path.content().replaceTextOrThrow("old"_el);
        REQUIRE(
            temporary->commitTo(path, el::path::PathCommitOptions{}.setCollisionMode(el::path::PathCollisionMode::Stop))
                .isFailure());
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "old"_el);
        temporary->close();
        REQUIRE(temporary->isEmpty());
    }

    void testFinalizationFailuresAndTimeoutRetry() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-finalize"};
        const auto source = fixture.child("temporary");
        const auto destination = fixture.child("file");
        source.content().writeTextOrThrow("new"_el);
        destination.content().writeTextOrThrow("old"_el);
        auto stream = ReplacementOutputStream{};
        auto engine = el::path::impl::FileCommit{source, destination, {}, stream.settings};
        stream.failFlush = true;
        try {
            engine.commit(&stream);
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            const auto diagnostic = el::err::DiagnosticHelper{error}.toDocument().toString();
            REQUIRE(diagnostic.contains("Flushing"_el));
            REQUIRE(diagnostic.contains("Deep flush cause"_el));
            REQUIRE(diagnostic.contains(source.toString()));
            REQUIRE(diagnostic.contains(destination.toString()));
        }
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
        stream.failFlush = false;
        stream.flushTimeout = true;
        REQUIRE_THROWS_AS(el::path::PathError, engine.commit(&stream));
        stream.flushTimeout = false;
        stream.failClose = true;
        REQUIRE_THROWS_AS(el::path::PathError, engine.commit(&stream));
        stream.failClose = false;
        stream.closeTimeout = true;
        REQUIRE_THROWS_AS(el::path::PathError, engine.commit(&stream));
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
        const auto flushCalls = stream.flushCalls;
        stream.closeTimeout = false;
        engine.commit(&stream);
        REQUIRE_EQUAL(stream.flushCalls, flushCalls);
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "new"_el);
    }

    void testMoveFailureAndNetworkCause() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-cause"};
        auto temporary = fixture.path().operations().openTempTextOutputStreamOrThrow();
        const auto source = temporary->path();
        const auto destination = fixture.child("file");
        auto options = el::path::PathCommitOptions{}.setBeforeCommit(
            [](const el::path::Path &sourcePath, const el::path::Path &) -> void {
                sourcePath.operations().removeOrThrow();
            });
        try {
            temporary->commitToOrThrow(destination, options);
            REQUIRE(false);
        } catch (const el::path::PathError &) {
            auto context =
                el::network::NetworkErrorContext{"HTTP output failed"_el, "Committing the response failed."_el};
            context.setCause(std::current_exception());
            const auto error = el::network::NetworkError{context};
            REQUIRE(error.hasCause());
            const auto rendered = el::err::DiagnosticHelper{error}.toDocument().toString();
            REQUIRE(rendered.contains("HTTP output failed"_el));
            REQUIRE(rendered.contains("Moving the completed temporary"_el));
            REQUIRE(rendered.contains("source path does not exist"_el));
            REQUIRE(rendered.contains(source.toString()));
        }
        REQUIRE_FALSE(destination.info().exists());
    }

    void testUnexpectedFinalizationExceptionsEscape() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-unexpected-finalize"};
        const auto destination = fixture.child("file");
        const auto source = fixture.child("temporary");
        destination.content().writeTextOrThrow("old"_el);
        source.content().writeTextOrThrow("new"_el);
        for (const auto failDuringFlush : {true, false}) {
            auto output = ReplacementOutputStream{};
            output.unexpectedFlush = failDuringFlush;
            output.unexpectedClose = !failDuringFlush;
            auto engine = el::path::impl::FileCommit{
                source, destination, el::path::PathCommitOptions{}.setIgnoreBackupErrors(true), output.settings};
            auto caught = false;
            try {
                engine.commit(&output);
            } catch (const std::bad_alloc &) {
                caught = true;
            }
            REQUIRE(caught);
            REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
            REQUIRE_EQUAL(source.content().readTextOrThrow(), "new"_el);
        }
    }

    void testUnexpectedExceptionsEscapeReplacement() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto fixture = PathTestFixture{"replace-unexpected"};
        const auto destination = fixture.child("file");
        destination.content().writeTextOrThrow("old"_el);
        auto temporary = fixture.path().operations().openTempTextOutputStreamOrThrow();
        REQUIRE(temporary->write("new"_el).isSuccess());
        auto options = el::path::PathCommitOptions{}.setIgnoreBackupErrors(true).setBeforeCommit(
            [](const el::path::Path &, const el::path::Path &) -> void { throw std::bad_alloc{}; });
        auto caughtAllocationFailure = false;
        try {
            temporary->commitToOrThrow(destination, options);
        } catch (const std::bad_alloc &) {
            caughtAllocationFailure = true;
        }
        REQUIRE(caughtAllocationFailure);
        REQUIRE_FALSE(temporary->isEmpty());
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
        options.setBeforeCommit([](const el::path::Path &, const el::path::Path &) -> void {
            throw el::err::LogicFailure{"Unexpected callback defect"_el};
        });
        auto caughtLogicFailure = false;
        try {
            destination.content().replaceTextOrThrow(
                "other"_el, el::path::PathReplaceTextOptions{}.setCommitOptions(options));
        } catch (const el::err::LogicFailure &error) {
            REQUIRE_EQUAL(error.reason(), "Unexpected callback defect"_el);
            caughtLogicFailure = true;
        }
        REQUIRE(caughtLogicFailure);
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "old"_el);
        temporary->commitToOrThrow(destination);
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "new"_el);
    }

private:
    /// Verify that native error category and code survive every wrapping layer.
    void requireNotFoundCause(const el::path::PathError &error) {
        auto cause = error.cause();
        auto found = false;
        while (cause != nullptr) {
            try {
                std::rethrow_exception(cause);
            } catch (const el::path::PathError &inner) {
                if (inner.platformContext() != nullptr) {
                    REQUIRE_EQUAL(inner.platformContext()->category(), el::system::PlatformErrorCategory::NotFound);
#if defined(_WIN32)
                    const auto native =
                        std::dynamic_pointer_cast<const el::system::impl::WindowsErrorContext>(inner.platformContext());
                    REQUIRE(native != nullptr);
                    REQUIRE_EQUAL(native->errorCode(), DWORD{ERROR_FILE_NOT_FOUND});
#else
                    const auto native =
                        std::dynamic_pointer_cast<const el::system::impl::PosixErrorContext>(inner.platformContext());
                    REQUIRE(native != nullptr);
                    REQUIRE_EQUAL(native->errorCode(), ENOENT);
#endif
                    found = true;
                }
                cause = inner.cause();
            }
        }
        REQUIRE(found);
    }
};
