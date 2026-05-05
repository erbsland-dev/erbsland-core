// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/FormatAs.hpp>
#include <erbsland/text/impl/FormatMakeArguments.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/U8Format.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/unit/ArgumentUnit.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpOffset.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <string_view>

using el::text::StringConverter;
using el::text::U8Format;
using el::unit::ArgumentIndex;
using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::CpOffset;

namespace erbsland::test {

struct CustomSignedValue {
    int32_t value;
};

struct CustomUnsignedValue {
    uint32_t value;
};

struct CustomTextValue {
    std::string value;
};

struct AmbiguousValue {
    int32_t value;
};

}

using el::test::AmbiguousValue;
using el::test::CustomSignedValue;
using el::test::CustomTextValue;
using el::test::CustomUnsignedValue;

namespace erbsland::text {

template <>
struct FormatAsInt64<test::CustomSignedValue> : FormatAs<test::CustomSignedValue, int64_t> {
    [[nodiscard]] auto format(const test::CustomSignedValue &value) const -> int64_t { return value.value; }
};

template <>
struct FormatAsUInt64<test::CustomUnsignedValue> : FormatAs<test::CustomUnsignedValue, uint64_t> {
    [[nodiscard]] auto format(const test::CustomUnsignedValue &value) const -> uint64_t { return value.value; }
};

template <>
struct FormatAsU8Text<test::CustomTextValue> : FormatAs<test::CustomTextValue, U8String> {
    [[nodiscard]] auto format(const test::CustomTextValue &value) const -> U8String {
        return U8String{std::string_view{value.value}};
    }
};

template <>
struct FormatAsInt64<test::AmbiguousValue> : FormatAs<test::AmbiguousValue, int64_t> {
    [[nodiscard]] auto format(const test::AmbiguousValue &value) const -> int64_t { return value.value; }
};

template <>
struct FormatAsUInt64<test::AmbiguousValue> : FormatAs<test::AmbiguousValue, uint64_t> {
    [[nodiscard]] auto format(const test::AmbiguousValue &value) const -> uint64_t {
        return static_cast<uint64_t>(value.value);
    }
};

}

TESTED_TARGETS(FormatAs U8Format)
class FormatAsTest final : public el::UnitTest {
public:
    void testCustomFormatAsSpecializations() {
        const auto format = U8Format{"{}|{}|{}"};

        const auto result = format.build(CustomSignedValue{-12}, CustomUnsignedValue{34U}, CustomTextValue{"name"});

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"-12|34|name"});
    }

    void testUnitValuesFormatAsRawNumbers() {
        const auto format = U8Format{"{}|{}|{}|{}"};

        const auto result = format.build(CpIndex{2U}, CpLength{3U}, CpOffset{-1}, ArgumentIndex{4U});

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"2|3|-1|4"});
    }

    void testAmbiguousFormatAsMatchCount() {
        REQUIRE_EQUAL(el::text::impl::formatAsMatchCount<AmbiguousValue>(), std::size_t{2U});
    }
};
