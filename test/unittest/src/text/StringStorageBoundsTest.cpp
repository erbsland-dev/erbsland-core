// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CharRange.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OutOfRangeFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/text/impl/CharSetRangeBuilder.hpp>
#include <erbsland/text/impl/RepeatCount.hpp>
#include <erbsland/text/u16/impl/U16Encoding.hpp>
#include <erbsland/text/u16/impl/U16StringSharedStorage.hpp>
#include <erbsland/text/u32/impl/U32Encoding.hpp>
#include <erbsland/text/u32/impl/U32StringSharedStorage.hpp>
#include <erbsland/text/u8/impl/U8Encoding.hpp>
#include <erbsland/text/u8/impl/U8StringSharedStorage.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <limits>
#include <span>

using el::err::OutOfRangeFailure;
using el::err::ParameterFailure;
using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(U8StringSharedStorage U16StringSharedStorage U32StringSharedStorage CharSetRangeBuilder)
class StringStorageBoundsTest final : public el::UnitTest {
public:
    void testStorageSizeBounds() {
        WITH_CONTEXT(verifyStorageBounds<el::text::impl::U8StringSharedStorage>());
        WITH_CONTEXT(verifyStorageBounds<el::text::impl::U16StringSharedStorage>());
        WITH_CONTEXT(verifyStorageBounds<el::text::impl::U32StringSharedStorage>());
    }

    void testRejectedAppendPreservesContent() {
        WITH_CONTEXT(verifyRejectedAppend<U8StringEditor>());
        WITH_CONTEXT(verifyRejectedAppend<U16StringEditor>());
        WITH_CONTEXT(verifyRejectedAppend<U32StringEditor>());
    }

    void testRepeatCountMustBeFinite() {
        REQUIRE_EQUAL(el::text::impl::repeatCountToSize(ItemCount{3U}), 3U);
        REQUIRE_THROWS_AS(ParameterFailure, el::text::impl::repeatCountToSize(ItemCount::infinite()));
        REQUIRE_THROWS_AS(ParameterFailure, el::text::impl::repeatCountToSize(CpCount::infinite()));
    }

    void testRepeatCountDiagnosticNamesParameter() {
        try {
            [[maybe_unused]] const auto count = el::text::impl::repeatCountToSize(ItemCount::infinite());
            REQUIRE(false);
        } catch (const ParameterFailure &failure) {
            REQUIRE_EQUAL(failure.toString(), "Repeated string count must be finite (parameter: count)"_el);
        }
    }

    void testCharacterSetCapacityFailure() {
        auto builder = el::text::impl::CharSetRangeBuilder{std::numeric_limits<std::size_t>::max()};
        builder.add(CharRange{Char{U'a'}, Char{U'a'}});
        builder.add(CharRange{Char{U'c'}, Char{U'c'}});
        REQUIRE_THROWS_AS(OutOfRangeFailure, builder.add(CharRange{Char{U'e'}, Char{U'e'}}));
    }

    void testDecoderPositionPreconditions() {
        auto bytePosition = ByteIndex::zero();
        auto u16Position = U16Index::zero();
        auto cpPosition = CpIndex::zero();
        REQUIRE_THROWS_AS(
            OutOfRangeFailure, el::text::impl::utf8::decodeCharOrThrow(std::span<const char>{}, bytePosition));
        REQUIRE_THROWS_AS(
            OutOfRangeFailure, el::text::impl::utf16::decodeCharOrThrow(std::span<const char16_t>{}, u16Position));
        REQUIRE_THROWS_AS(
            OutOfRangeFailure, el::text::impl::utf32::decodeCharOrThrow(std::span<const char32_t>{}, cpPosition));
        REQUIRE_EQUAL(bytePosition, ByteIndex::zero());
        REQUIRE_EQUAL(u16Position, U16Index::zero());
        REQUIRE_EQUAL(cpPosition, CpIndex::zero());
    }

private:
    template <typename T>
    void verifyStorageBounds() {
        constexpr auto nativeMaximum = std::numeric_limits<std::size_t>::max();
        auto finiteMaximum = nativeMaximum;
        if (finiteMaximum > ByteCount::cRawMaximum) {
            finiteMaximum = static_cast<std::size_t>(ByteCount::cRawMaximum);
        }
        REQUIRE_EQUAL(T::checkedAddSize(2U, 3U), 5U);
        REQUIRE_EQUAL(T::checkedMultiplySize(2U, 3U), 6U);
        REQUIRE_EQUAL(T::checkedMultiplySize(nativeMaximum, 0U), 0U);
        REQUIRE_EQUAL(T::checkedAddSize(finiteMaximum - 1U, 1U), finiteMaximum);
        REQUIRE_EQUAL(T::checkedMultiplySize(finiteMaximum, 1U), finiteMaximum);
        REQUIRE_THROWS_AS(OutOfRangeFailure, T::checkedAddSize(nativeMaximum, 1U));
        REQUIRE_THROWS_AS(OutOfRangeFailure, T::checkedMultiplySize(nativeMaximum, 2U));
        if (nativeMaximum > finiteMaximum) {
            REQUIRE_THROWS_AS(OutOfRangeFailure, T::validateSize(nativeMaximum));
            REQUIRE_THROWS_AS(OutOfRangeFailure, T::checkedAddSize(finiteMaximum, 1U));
            REQUIRE_THROWS_AS(OutOfRangeFailure, T::checkedMultiplySize(nativeMaximum, 1U));
        }
        try {
            [[maybe_unused]] const auto unexpectedResult = T::checkedAddSize(nativeMaximum, 1U);
            REQUIRE(false);
        } catch (const OutOfRangeFailure &failure) {
            REQUIRE_EQUAL(failure.reason(), "String storage size exceeds bounds"_el);
        }
    }

    template <typename T>
    void verifyRejectedAppend() {
        auto text = T{};
        text.append(Char{U'A'});
        REQUIRE_THROWS_AS(ParameterFailure, text.append(Char{U'B'}, CpCount::infinite()));
        REQUIRE_THROWS_AS(OutOfRangeFailure, text.append(text, ItemCount::maximum()));
        REQUIRE_EQUAL(StringConverter{text}.toU8String(), "A"_el);
    }
};
