// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/FormatAs.hpp>
#include <erbsland/text/impl/FormatMakeArguments.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/u8/U8Format.hpp>
#include <erbsland/unit/ArgumentUnit.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpOffset.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using el::text::StringConverter;
using el::text::U8Format;
using el::unit::ArgumentIndex;
using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::CpOffset;

namespace erbsland::test {

struct CustomTextValue {
    std::string value;

    [[nodiscard]] auto toString() const -> el::text::String { return el::text::String{std::string_view{"ignored"}}; }
};

struct ToStringValue {
    std::string value;

    [[nodiscard]] auto toString() const -> el::text::String { return el::text::String{std::string_view{value}}; }
};

}

using el::test::CustomTextValue;
using el::test::ToStringValue;

namespace erbsland::text {

template <>
struct FormatAs<test::CustomTextValue> {
    [[nodiscard]] auto format(const test::CustomTextValue &value) const -> String {
        return String{StringEditor{std::string_view{value.value}}};
    }
};

}

TESTED_TARGETS(FormatAs U8Format)
class FormatAsTest final : public el::UnitTest {
public:
    void testFormatAsOverridesToString() {
        const auto format = U8Format{"{}"};

        const auto result = format.build(CustomTextValue{"name"});

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"name"});
    }

    void testCustomToString() {
        const auto format = U8Format{"{}"};

        const auto result = format.build(ToStringValue{"name"});

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"name"});
    }

    void testUnitValuesFormatAsRawNumbers() {
        const auto format = U8Format{"{}|{}|{}|{}"};

        const auto result = format.build(CpIndex{2U}, CpLength{3U}, CpOffset{-1}, ArgumentIndex{4U});

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"2|3|-1|4"});
    }

    void testByteBlockFormatArgument() {
        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0x01U, 0xabU});
        const auto argument = el::text::impl::makeFormatArgument(bytes);

        REQUIRE_EQUAL(argument.kind(), el::text::FormatArgumentKind::Bytes);
        REQUIRE_EQUAL(argument.bytes(), bytes);
        REQUIRE_EQUAL(StringConverter{U8Format{"{}"}.build(bytes)}.toStdString(), std::string{"01ab"});
    }
};
