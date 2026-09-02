// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/system/impl/PosixErrorContext.hpp>
#include <erbsland/system/PlatformError.hpp>
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
        const auto capturedErrno = el::system::impl::PosixErrorContext::fromErrno();
        errno = previousErrno;
        const auto errorCode = capturedErrno->errorCode();
        REQUIRE_EQUAL(errorCode, ENOENT);
        const auto context = el::system::impl::PosixErrorContext::fromErrorCode(2);
        const auto category = context->category();
        REQUIRE_EQUAL(category, el::system::PlatformErrorCategory::NotFound);
        REQUIRE_EQUAL(
            el::system::impl::PosixErrorContext{EACCES}.category(),
            el::system::PlatformErrorCategory::PermissionDenied);
        REQUIRE_EQUAL(
            el::system::impl::PosixErrorContext{EEXIST}.category(), el::system::PlatformErrorCategory::AlreadyExists);
        REQUIRE_EQUAL(el::system::impl::PosixErrorContext{-1}.category(), el::system::PlatformErrorCategory::Unknown);
        const auto error = el::system::PlatformError{"Native lookup failed"_el, context};
        REQUIRE_EQUAL(error.context(), context);
        const auto text = el::text::StringConverter{error.diagnostic()->toTextDocument().toString()}.toStdString();
        const auto lookupFailedPosition = text.find("Native lookup failed");
        const auto errnoPosition = text.find("errno");
        REQUIRE_NOT_EQUAL(lookupFailedPosition, std::string::npos);
        REQUIRE_NOT_EQUAL(errnoPosition, std::string::npos);
    }
};
