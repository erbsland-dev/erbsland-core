// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/system/PlatformError.hpp>
#include <erbsland/system/PosixErrorContext.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cerrno>

using namespace el::text::literals;

TESTED_TARGETS(PosixErrorContext PlatformError PlatformErrorCategory)
class PosixDiagnosticTest final : public el::UnitTest {
public:
    void testCategoryMappingAndPlatformError() {
        const auto previousErrno = errno;
        errno = ENOENT;
        const auto capturedErrno = el::system::PosixErrorContext::fromErrno();
        errno = previousErrno;
        REQUIRE_EQUAL(capturedErrno->errorCode(), ENOENT);
        const auto context = el::system::PosixErrorContext::fromErrorCode(2);
        REQUIRE_EQUAL(context->category(), el::system::PlatformErrorCategory::NotFound);
        REQUIRE_EQUAL(
            el::system::PosixErrorContext{EACCES}.category(), el::system::PlatformErrorCategory::PermissionDenied);
        REQUIRE_EQUAL(
            el::system::PosixErrorContext{EEXIST}.category(), el::system::PlatformErrorCategory::AlreadyExists);
        REQUIRE_EQUAL(el::system::PosixErrorContext{-1}.category(), el::system::PlatformErrorCategory::Unknown);
        const auto error = el::system::PlatformError{"Native lookup failed"_el, context};
        REQUIRE_EQUAL(error.context(), context);
        const auto text = el::text::StringConverter{error.diagnostic()->toTextDocument().toString()}.toStdString();
        REQUIRE(text.find("Native lookup failed") != std::string::npos);
        REQUIRE(text.find("errno") != std::string::npos);
    }
};
