// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/system/impl/WindowsErrorContext.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(WindowsErrorContext PlatformErrorCategory)
class WindowsDiagnosticTest final : public el::UnitTest {
public:
    void testCategoryMappingAndUnknownFallback() {
        REQUIRE_EQUAL(
            el::system::impl::WindowsErrorContext{ERROR_FILE_NOT_FOUND}.category(),
            el::system::PlatformErrorCategory::NotFound);
        REQUIRE_EQUAL(
            el::system::impl::WindowsErrorContext{ERROR_ACCESS_DENIED}.category(),
            el::system::PlatformErrorCategory::PermissionDenied);
        REQUIRE_EQUAL(
            el::system::impl::WindowsErrorContext{ERROR_ALREADY_EXISTS}.category(),
            el::system::PlatformErrorCategory::AlreadyExists);
        REQUIRE_EQUAL(
            el::system::impl::WindowsErrorContext{0xffffffffU}.category(), el::system::PlatformErrorCategory::Unknown);
    }
};
