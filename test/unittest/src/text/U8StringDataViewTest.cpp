// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/u8/impl/U8StringDataView.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <span>

using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(U8StringDataView)
class U8StringDataViewTest final : public el::UnitTest {
public:
    void testStoresDataAndRange() {
        const auto text = std::array<char, 5>{'H', 'e', 'l', 'l', 'o'};
        const auto range = ByteRange{ByteIndex{1U}, ByteLength{3U}};

        const auto view = el::text::impl::U8StringDataView{std::span<const char>{text}, range};

        REQUIRE_EQUAL(view.data().size(), text.size());
        REQUIRE_EQUAL(view.data()[1], 'e');
        REQUIRE_EQUAL(view.range(), range);
    }

    void testDataSpanReturnsSelectedRange() {
        const auto text = std::array<char, 5>{'H', 'e', 'l', 'l', 'o'};
        const auto range = ByteRange{ByteIndex{1U}, ByteLength{3U}};

        const auto span = el::text::impl::U8StringDataView{std::span<const char>{text}, range}.dataSpan();

        REQUIRE_EQUAL(span.size(), 3U);
        REQUIRE_EQUAL(span[0], 'e');
        REQUIRE_EQUAL(span[2], 'l');
    }

    void testDataSpanClampsToAvailableData() {
        const auto text = std::array<char, 5>{'H', 'e', 'l', 'l', 'o'};
        const auto range = ByteRange{ByteIndex{3U}, ByteLength{5U}};

        const auto span = el::text::impl::U8StringDataView{std::span<const char>{text}, range}.dataSpan();

        REQUIRE_EQUAL(span.size(), 2U);
        REQUIRE_EQUAL(span[0], 'l');
        REQUIRE_EQUAL(span[1], 'o');
    }

    void testRelativeRangeForAbsolute() {
        const auto text = std::array<char, 5>{'H', 'e', 'l', 'l', 'o'};
        const auto range = ByteRange{ByteIndex{1U}, ByteLength{4U}};
        const auto view = el::text::impl::U8StringDataView{std::span<const char>{text}, range};
        const auto absoluteRange = ByteRange{ByteIndex{3U}, ByteLength{3U}};
        const auto expectedRange = ByteRange{ByteIndex{2U}, ByteLength{2U}};

        const auto relativeRange = view.relativeRangeForAbsolute(absoluteRange);

        REQUIRE_EQUAL(relativeRange, expectedRange);
    }
};
