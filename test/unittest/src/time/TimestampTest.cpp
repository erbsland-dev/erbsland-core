// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/time/Timestamp.hpp>
#include <erbsland/unit/StdFormat.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <chrono>
#include <limits>
#include <span>
#include <type_traits>

using namespace el::time;
using namespace el::text::literals;

TESTED_TARGETS(Timestamp)
class TimestampTest final : public el::UnitTest {
public:
    void testRepresentation() {
        static_assert(std::is_trivially_copyable_v<Timestamp>);
        static_assert(std::is_standard_layout_v<Timestamp>);
        static_assert(std::is_same_v<Timestamp::RawValue, std::pair<int32_t, int64_t>>);
        REQUIRE_FALSE(Timestamp{}.isValid());
        REQUIRE_FALSE(Timestamp{}.isValidDateTime());
        REQUIRE_EQUAL(Timestamp{}.toRawValue(), (Timestamp::RawValue{-1, 0}));
        REQUIRE(Timestamp::first().isValid());
        REQUIRE_EQUAL(Timestamp::first().toRawValue(), (Timestamp::RawValue{0, 0}));
        REQUIRE_EQUAL(Timestamp::last().toRawValue(), (Timestamp::RawValue{3'652'424, 86'399'999'999'999}));
        REQUIRE_EQUAL(Timestamp{}.dateAsDays(), Days{-1});
        REQUIRE_FALSE(Timestamp{}.date().isValid());
        REQUIRE_EQUAL(Timestamp{}.timeAsNanoseconds(), Nanoseconds{});
        REQUIRE_EQUAL(Timestamp{}.time(), Time{});
        REQUIRE_FALSE((Timestamp{Date{}, Time::last()}).isValid());
        REQUIRE_EQUAL((Timestamp{Date{}, Time::last()}), Timestamp{});
    }

    void testRawValidation() {
        const auto cMinimum = std::numeric_limits<int64_t>::min();
        REQUIRE_EQUAL(Timestamp::fromRawValueOrThrow(-1, 500), Timestamp{});
        REQUIRE_EQUAL(Timestamp::fromRawValueOrThrow(42, cMinimum), Timestamp{});
        REQUIRE_EQUAL(Timestamp::fromRawValueOrThrow(-10, cMinimum), Timestamp{});
        REQUIRE_FALSE(Timestamp::fromRawValue(3'652'425, 0));
        REQUIRE_FALSE(Timestamp::fromRawValue(0, 86'400'000'000'000));
        REQUIRE_THROWS_AS(el::err::ParameterError, Timestamp::fromRawValueOrThrow(3'652'425, 0));
        REQUIRE_THROWS_AS(el::err::ParameterError, Timestamp::fromRawValueOrThrow(0, 86'400'000'000'000));
        REQUIRE_FALSE(Timestamp::fromDaysAndNanoseconds(Days{std::numeric_limits<int64_t>::max()}, Nanoseconds{}));
        REQUIRE_EQUAL(Timestamp::fromDaysAndNanosecondsOrThrow(Days{-1}, Nanoseconds{100}), Timestamp{});
        REQUIRE_EQUAL(Timestamp::fromRawValueOrThrow(Timestamp::last().toRawValue()), Timestamp::last());
    }

    void testSerialization() {
        const auto invalid = Timestamp{}.toByteBlock();
        REQUIRE_EQUAL(invalid, bytesFromHex("ffffffff0000000000000000"_el));
        REQUIRE_EQUAL(Timestamp::first().toByteBlock(), bytesFromHex("000000000000000000000000"_el));
        const auto sample = Timestamp::fromRawValueOrThrow(0x010203, 0x010203040506LL);
        REQUIRE_EQUAL(sample.toByteBlock(), bytesFromHex("000102030000010203040506"_el));
        for (const auto timestamp : std::array{Timestamp{}, Timestamp::first(), sample, Timestamp::last()}) {
            REQUIRE_EQUAL(timestamp.toByteBlock().length(), el::unit::ByteLength{12U});
            REQUIRE_EQUAL(Timestamp::fromByteBlockOrThrow(timestamp.toByteBlock()), timestamp);
        }
        REQUIRE_FALSE(Timestamp::fromByteBlock(el::mem::ByteBlock{}));
        REQUIRE_FALSE(Timestamp::fromByteBlock(bytesFromHex("0000000000000000000000"_el)));
        REQUIRE_FALSE(Timestamp::fromByteBlock(bytesFromHex("00000000000000000000000000"_el)));
        REQUIRE_THROWS_AS(el::err::ParameterError, Timestamp::fromByteBlockOrThrow(el::mem::ByteBlock{}));
        REQUIRE_EQUAL(Timestamp::fromByteBlockOrThrow(bytesFromHex("00000001ffffffffffffffff"_el)), Timestamp{});
        REQUIRE_FALSE(Timestamp::fromByteBlock(bytesFromHex("7fffffff0000000000000000"_el)));
    }

    void testOrdering() {
        const auto values = std::array{
            Timestamp{},
            Timestamp::first(),
            Timestamp::fromRawValueOrThrow(0, 1),
            Timestamp::fromRawValueOrThrow(1, 0),
            Timestamp::last()};
        for (std::size_t index = 1; index < values.size(); ++index) {
            REQUIRE_LESS(values[index - 1], values[index]);
            if (index > 1) {
                REQUIRE_LESS(values[index - 1].toByteBlock(), values[index].toByteBlock());
            }
        }
        REQUIRE_GREATER(Timestamp{}.toByteBlock(), Timestamp::last().toByteBlock());
    }

    void testNowAndFormatting() {
        const auto before = std::chrono::system_clock::now();
        const auto timestamp = Timestamp::now();
        const auto after = std::chrono::system_clock::now();
        REQUIRE(timestamp.isValid());
        const auto parts = timestamp.toSecondsAndFractionsOrThrow(TimeEpoch::Posix);
        const auto captured =
            std::chrono::seconds{parts.first.toRawValue()} + std::chrono::nanoseconds{parts.second.toRawValue()};
        REQUIRE(captured >= before.time_since_epoch());
        REQUIRE(captured <= after.time_since_epoch());
        REQUIRE_EQUAL(Timestamp::first().toString(), "0000-01-01T00:00:00.000000000Z"_el);
        REQUIRE_EQUAL(Timestamp::last().toString(), "9999-12-31T23:59:59.999999999Z"_el);
        REQUIRE(Timestamp{}.toString().isEmpty());
        REQUIRE_EQUAL(el::text::StringFormat{"{}"_el}.build(Timestamp::first()), Timestamp::first().toString());
    }

private:
    /// Decode fixture bytes using the unit-test hex helper.
    [[nodiscard]] static auto bytesFromHex(const el::text::String &text) -> el::mem::ByteBlock {
        const auto bytes = el::unittest::th::stdStringFromHex(el::text::StringConverter{text}.toStdString());
        return el::mem::ByteBlock::fromSpan(std::span<const char>{bytes});
    }
};
