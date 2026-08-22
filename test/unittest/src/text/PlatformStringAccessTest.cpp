// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/Exception.hpp>
#include <erbsland/text/impl/PlatformU16StringAccess.hpp>
#include <erbsland/text/impl/PlatformU8StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU16StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/U16DataIndex.hpp>
#include <erbsland/unit/U16DataLength.hpp>
#include <erbsland/unit/U16DataRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstring>

using namespace el::text::literals;

TESTED_TARGETS(Exception PlatformU8StringAccess PlatformU16StringAccess U8StringDataView U16StringDataView)
class PlatformStringAccessTest final : public el::UnitTest {
public:
    void testEmptyStringsHaveNullTerminators() {
        const auto u8Access = el::text::impl::PlatformU8StringAccess{el::text::String{}};
        const auto u16Access = el::text::impl::PlatformU16StringAccess{el::text::U16String{}};

        REQUIRE_NOT_EQUAL(u8Access.nullTerminatedCharPtr(), nullptr);
        REQUIRE_EQUAL(u8Access.nullTerminatedCharPtr()[0], '\0');
        REQUIRE_EQUAL(u8Access.sizeWithoutNull(), 0U);
        REQUIRE_EQUAL(u8Access.sizeIncludingNull(), 1U);
        REQUIRE_NOT_EQUAL(u16Access.nullTerminatedCharPtr(), nullptr);
        REQUIRE_EQUAL(u16Access.nullTerminatedCharPtr()[0], u'\0');
        REQUIRE_EQUAL(u16Access.sizeWithoutNull(), 0U);
        REQUIRE_EQUAL(u16Access.sizeIncludingNull(), 1U);
    }

    void testCompleteStringsShareData() {
        const auto u8String = el::text::String{"complete"_el};
        const auto u16String = el::text::U16String{std::u16string_view{u"complete"}};
        const auto u8Access = el::text::impl::PlatformU8StringAccess{u8String};
        const auto u16Access = el::text::impl::PlatformU16StringAccess{u16String};

        REQUIRE(!el::text::impl::UnsafeU8StringAccess{u8String}.dataView().isSlice());
        REQUIRE(!el::text::impl::UnsafeU16StringAccess{u16String}.dataView().isSlice());
        REQUIRE_EQUAL(
            u8Access.nullTerminatedCharPtr(), el::text::impl::UnsafeU8StringAccess{u8String}.dataSpan().data());
        REQUIRE_EQUAL(
            u16Access.nullTerminatedCharPtr(), el::text::impl::UnsafeU16StringAccess{u16String}.dataSpan().data());
    }

    void testU8SliceIsMaterializedAndNullTerminated() {
        const auto source = el::text::String{"xplatform-tail"_el};
        const auto slice = source.slice(el::unit::ByteRange{el::unit::ByteIndex{1U}, el::unit::ByteLength{8U}});
        const auto access = el::text::impl::PlatformU8StringAccess{slice};

        REQUIRE(el::text::impl::UnsafeU8StringAccess{slice}.dataView().isSlice());
        REQUIRE_NOT_EQUAL(
            access.nullTerminatedCharPtr(), el::text::impl::UnsafeU8StringAccess{slice}.dataSpan().data());
        REQUIRE_EQUAL(std::strcmp(access.nullTerminatedCharPtr(), "platform"), 0);
        REQUIRE_EQUAL(access.sizeWithoutNull(), 8U);
        REQUIRE_EQUAL(access.sizeIncludingNull(), 9U);
        REQUIRE_EQUAL(access.nullTerminatedCharPtr()[access.sizeWithoutNull()], '\0');
    }

    void testU16SliceIsMaterializedAndNullTerminated() {
        const auto source = el::text::U16String{std::u16string_view{u"xplatform-tail"}};
        const auto slice =
            source.slice(el::unit::U16DataRange{el::unit::U16DataIndex{1U}, el::unit::U16DataLength{8U}});
        const auto access = el::text::impl::PlatformU16StringAccess{slice};

        REQUIRE(el::text::impl::UnsafeU16StringAccess{slice}.dataView().isSlice());
        REQUIRE_NOT_EQUAL(
            access.nullTerminatedCharPtr(), el::text::impl::UnsafeU16StringAccess{slice}.dataSpan().data());
        REQUIRE_EQUAL(std::char_traits<char16_t>::compare(access.nullTerminatedCharPtr(), u"platform", 8U), 0);
        REQUIRE_EQUAL(access.sizeWithoutNull(), 8U);
        REQUIRE_EQUAL(access.sizeIncludingNull(), 9U);
        REQUIRE_EQUAL(access.nullTerminatedCharPtr()[access.sizeWithoutNull()], u'\0');
    }

    void testExceptionMaterializesSlicedReason() {
        const auto source = el::text::String{"xreason-tail"_el};
        const auto slice = source.slice(el::unit::ByteRange{el::unit::ByteIndex{1U}, el::unit::ByteLength{6U}});
        const auto error = el::err::Exception{slice};

        REQUIRE_EQUAL(std::strcmp(error.what(), "reason"), 0);
        REQUIRE_EQUAL(error.reason(), slice);
        REQUIRE_NOT_EQUAL(error.what(), el::text::impl::UnsafeU8StringAccess{slice}.dataSpan().data());
    }
};
