// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/impl/U8StringLiteralStorage.hpp>
#include <erbsland/text/u8/impl/U8StringReadTools.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <span>
#include <string>

using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(U8StringLiteralStorage)
class U8StringLiteralStorageTest final : public el::UnitTest {
public:
    void testDefaultStorageIsEmpty() {
        const auto storage = el::text::impl::U8StringLiteralStorage{};

        REQUIRE(storage.data().empty());
        REQUIRE(storage.range().isEmpty());
        REQUIRE_EQUAL(storage.size(), 0U);
        REQUIRE(toString(storage.dataView()).empty());
    }

    void testStorageFromSpanUsesFullRange() {
        constexpr auto text = std::array<char, 7>{'l', 'i', 't', 'e', 'r', 'a', 'l'};
        const auto storage = el::text::impl::U8StringLiteralStorage{std::span<const char>{text}};

        REQUIRE_EQUAL(storage.data().size(), 7U);
        REQUIRE_EQUAL(storage.size(), 7U);
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(7U));
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"literal"});
    }

    void testStorageFromSpanWithCustomRange() {
        constexpr auto text = std::array<char, 7>{'l', 'i', 't', 'e', 'r', 'a', 'l'};
        const auto range = ByteRange{ByteIndex{2U}, ByteLength{3U}};
        const auto storage = el::text::impl::U8StringLiteralStorage{std::span<const char>{text}, range};

        REQUIRE_EQUAL(storage.range(), range);
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"ter"});
    }

    void testStorageFromCharPointer() {
        const auto *text = "literal";
        const auto storage = el::text::impl::U8StringLiteralStorage{text, 7U};

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"literal"});
    }

    void testStorageFromChar8Pointer() {
        const auto *text = u8"literal";
        const auto storage = el::text::impl::U8StringLiteralStorage{text, 7U};

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"literal"});
    }

    void testDataViewWithCustomRange() {
        const auto *text = "literal";
        const auto storage = el::text::impl::U8StringLiteralStorage{text, 7U};
        const auto range = ByteRange{ByteIndex{1U}, ByteLength{2U}};

        REQUIRE_EQUAL(toString(storage.dataView(range)), std::string{"it"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(7U));
    }

private:
    [[nodiscard]] static auto toString(const el::text::impl::U8StringDataView &view) noexcept -> std::string {
        return el::text::impl::U8StringReadTools{view}.toStdString();
    }
};
