// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ArgumentIndex.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpOffset.hpp>
#include <erbsland/FormatAs.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/text/impl/FormatMakeArguments.hpp>
#include <erbsland/U8Format.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using el::text::StringConverter;
using el::text::U8Format;
using el::unit::ArgumentIndex;
using el::unit::CpCount;
using el::unit::CpIndex;
using el::unit::CpOffset;

namespace erbsland::test {

using namespace text;

struct CustomTextValue {
    std::string value;

    [[nodiscard]] auto toString() const -> String { return String{std::string_view{"ignored"}}; }
};

struct ToStringValue {
    std::string value;

    [[nodiscard]] auto toString() const -> String { return String{std::string_view{value}}; }
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

        const auto result = format.build(CpIndex{2U}, CpCount{3U}, CpOffset{-1}, ArgumentIndex{4U});

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
