// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include <erbsland/path/impl/BackendFactory.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/system/PlatformErrorCategory.hpp>
#include <erbsland/system/PosixErrorContext.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cerrno>
#include <memory>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathOperations PosixPathBackend PathBackend)
class PosixPathOperationsTest final : public el::UnitTest {
public:
    void testNativeFailuresPreservePlatformContext() {
        const auto fixture = PathTestFixture{"posix-operation-errors"};
        const auto missing = fixture.child("missing");
        try {
            static_cast<void>(el::path::impl::pathBackend().directoryEntriesOrThrow(missing, missing));
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            requirePosixError(error, ENOENT);
            REQUIRE_EQUAL(toStdString(error.sourcePath()), toStdString(missing));
        }

        const auto source = fixture.child("source.txt");
        const auto destination = fixture.child("missing/destination.txt");
        source.content().writeTextOrThrow("data"_el);
        try {
            el::path::impl::pathBackend().copyFileEntryOrThrow(source, destination);
            REQUIRE(false);
        } catch (const el::path::PathError &error) {
            requirePosixError(error, ENOENT);
            REQUIRE_EQUAL(toStdString(error.sourcePath()), toStdString(source));
            REQUIRE_EQUAL(toStdString(error.targetPath()), toStdString(destination));
        }
    }

private:
    void requirePosixError(const el::path::PathError &error, const int expectedCode) {
        const auto context = std::dynamic_pointer_cast<const el::system::PosixErrorContext>(error.platformContext());
        REQUIRE(context != nullptr);
        REQUIRE_EQUAL(context->errorCode(), expectedCode);
        REQUIRE_EQUAL(context->category(), el::system::PlatformErrorCategory::NotFound);
    }
};
