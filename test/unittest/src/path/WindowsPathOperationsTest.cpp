// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include <erbsland/core/impl/WindowsApi.hpp>
#include <erbsland/path/impl/BackendFactory.hpp>
#include <erbsland/path/impl/PathBackend.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/system/PlatformErrorCategory.hpp>
#include <erbsland/system/WindowsErrorContext.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathOperations WindowsPathBackend PathBackend)
class WindowsPathOperationsTest final : public el::UnitTest {
public:
    void testNativeMoveReplacesRegularFile() {
        const auto fixture = PathTestFixture{"windows-native-replace"};
        const auto source = fixture.child("source.txt");
        const auto destination = fixture.child("destination.txt");
        source.content().writeTextOrThrow("replacement"_el);
        destination.content().writeTextOrThrow("previous"_el);

        el::path::impl::pathBackend().moveEntryOrThrow(source, destination);

        REQUIRE_FALSE(source.info().exists());
        REQUIRE_EQUAL(destination.content().readTextOrThrow(), "replacement"_el);
    }

    void testNativeFailuresPreservePlatformContext() {
        const auto fixture = PathTestFixture{"windows-operation-errors"};
        const auto missing = fixture.child("missing");
        try {
            static_cast<void>(el::path::impl::pathBackend().directoryEntriesOrThrow(missing, missing));
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            requireNotFoundError(error);
            REQUIRE_EQUAL(toStdString(error.sourcePath()), toStdString(missing));
        }

        const auto source = fixture.child("source.txt");
        const auto destination = fixture.child("missing/destination.txt");
        source.content().writeTextOrThrow("data"_el);
        try {
            el::path::impl::pathBackend().copyFileEntryOrThrow(source, destination);
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            requireNotFoundError(error);
            REQUIRE_EQUAL(toStdString(error.sourcePath()), toStdString(source));
            REQUIRE_EQUAL(toStdString(error.targetPath()), toStdString(destination));
        }
    }

    void testNativeSymbolicLinks() {
        const auto fixture = PathTestFixture{"windows-operation-symlinks"};
        const auto target = fixture.child("target.txt");
        target.content().writeTextOrThrow("data"_el);

        const auto relativeLink = fixture.child("relative-link");
        try {
            el::path::impl::pathBackend().createSymlinkOrThrow(el::path::Path{"target.txt"_el}, relativeLink, false);
        } catch (const el::path::PathError &error) {
            const auto context =
                std::dynamic_pointer_cast<const el::system::WindowsErrorContext>(error.platformContext());
            REQUIRE(context);
            REQUIRE_EQUAL(context->errorCode(), static_cast<DWORD>(ERROR_PRIVILEGE_NOT_HELD));
            return;
        }
        REQUIRE_EQUAL(
            toStdString(el::path::impl::pathBackend().readSymlinkOrThrow(relativeLink)), std::string{"target.txt"});

        const auto absoluteLink = fixture.child("absolute-link");
        el::path::impl::pathBackend().createSymlinkOrThrow(target, absoluteLink, false);
        REQUIRE_EQUAL(toStdString(el::path::impl::pathBackend().readSymlinkOrThrow(absoluteLink)), toStdString(target));
    }

private:
    void requireNotFoundError(const el::path::PathError &error) {
        const auto context = std::dynamic_pointer_cast<const el::system::WindowsErrorContext>(error.platformContext());
        REQUIRE(context);
        REQUIRE_EQUAL(context->category(), el::system::PlatformErrorCategory::NotFound);
    }
};
