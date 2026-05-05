// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/IntegerRange.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringViewList.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/HashSet.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/Set.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

using el::math::IntegerRange;
using el::math::orderMinimumMaximum;
using el::random::Random;
using el::text::CharSet;
using el::text::StringViewList;
using el::unit::ByteLength;
using el::unit::CpLength;
using el::unit::ElementCount;
using el::util::HashSet;
using el::util::List;
using el::util::Set;
using namespace el::text::literals;

namespace erbsland::test::randomtest {

class CountingRandom final : public Random {
public:
    [[nodiscard]] auto getInt32(const int32_t minimum, const int32_t maximum) -> int32_t override {
        return static_cast<int32_t>(getInt64(minimum, maximum));
    }
    [[nodiscard]] auto getUInt32(const uint32_t minimum, const uint32_t maximum) -> uint32_t override {
        return static_cast<uint32_t>(getUInt64(minimum, maximum));
    }
    [[nodiscard]] auto getInt64(int64_t minimum, int64_t maximum) -> int64_t override {
        orderMinimumMaximum(minimum, maximum);
        const auto range = static_cast<uint64_t>(maximum - minimum);
        return static_cast<int64_t>(minimum + static_cast<int64_t>(next(range + 1U)));
    }
    [[nodiscard]] auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t override {
        orderMinimumMaximum(minimum, maximum);
        const auto range = maximum - minimum;
        return minimum + next(range + 1U);
    }
    [[nodiscard]] auto getDouble(const double minimum, const double maximum) -> double override {
        return minimum <= maximum ? minimum : maximum;
    }
    [[nodiscard]] auto getBool() -> bool override { return next(2U) == 1U; }
    void fillBytes(const std::span<std::byte> destination) override {
        for (auto &byte : destination) {
            byte = static_cast<std::byte>(next(256U));
        }
    }

private:
    [[nodiscard]] auto next(const uint64_t modulo) -> uint64_t {
        const auto result = _next % modulo;
        ++_next;
        return result;
    }

private:
    uint64_t _next{0U};
};

}

using namespace erbsland::test::randomtest;

TESTED_TARGETS(Random)
class RandomTest final : public el::UnitTest {
public:
    void testIntegerConvenience() {
        auto random = CountingRandom{};

        REQUIRE_EQUAL(random.selectInteger<int>(10, 12), 10);
        REQUIRE_EQUAL(random.selectInteger<int>(12, 10), 11);
        REQUIRE_EQUAL(random.selectInteger(IntegerRange<int>{20, 22}), 22);

        const auto list = random.buildIntegerList(ElementCount{4}, 3, 5);
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{3, 4, 5, 3}));
    }

    void testEmptyInputs() {
        auto random = CountingRandom{};

        REQUIRE(random.selectIndex(ElementCount::zero()).isNoIndex());
        REQUIRE(random.selectIndex(ElementCount::infinite()).isNoIndex());
        REQUIRE_EQUAL(random.selectElement(std::vector<int>{}, 99), 99);
        REQUIRE(random.buildIntegerList(ElementCount::zero(), 1, 3).count().isZero());
        REQUIRE(random.buildString(CpLength{4}, CharSet{}).isEmpty());
        REQUIRE(random.buildByteBlock(ByteLength::zero()).isEmpty());
    }

    void testStringAndBytes() {
        auto random = CountingRandom{};

        const auto text = random.buildString(CpLength{3}, CharSet::fromPattern("A-C"_elv));
        REQUIRE(text == "ABC"_el);

        const auto block = random.buildByteBlock(ByteLength{4});
        REQUIRE_EQUAL(block.length(), ByteLength{4});
        REQUIRE_EQUAL(block.toUInt8Vector(), (std::vector<uint8_t>{3U, 4U, 5U, 6U}));
    }

    void testElementSelection() {
        auto random = CountingRandom{};
        const auto choices = List<int>{{1, 2, 3}};
        const auto setChoices = Set<int>{{4, 5, 6}};
        const auto hashSetChoices = HashSet<int>{{7, 8, 9}};

        REQUIRE_EQUAL(random.selectElement(choices), 1);
        REQUIRE(setChoices.contains(random.selectElement(setChoices)));
        REQUIRE(hashSetChoices.contains(random.selectElement(hashSetChoices)));

        auto listRandom = CountingRandom{};
        REQUIRE_EQUAL(
            listRandom.buildElementList(ElementCount{4}, choices).toStdVector(), (std::vector<int>{1, 2, 3, 1}));
        REQUIRE_EQUAL(random.buildElementList(ElementCount{2}, hashSetChoices).count(), ElementCount{2});

        const auto unique = random.buildUniqueElementList(ElementCount{2}, choices);
        REQUIRE_EQUAL(unique.count(), ElementCount{2});
        REQUIRE(unique.toStdSet().size() == 2U);
        REQUIRE_EQUAL(random.buildUniqueElementList(ElementCount{2}, hashSetChoices).count(), ElementCount{2});
    }

    void testDerivedListElementSelection() {
        auto choices = StringViewList{"red"_el, "green"_el, "blue"_el};

        auto random = CountingRandom{};
        REQUIRE(random.selectElement(choices) == "red"_el);
        REQUIRE(random.selectElement(StringViewList{}, "white"_elv) == "white"_el);

        auto listRandom = CountingRandom{};
        auto sample = listRandom.buildElementList(ElementCount{4}, choices);
        static_assert(std::is_same_v<decltype(sample), StringViewList>);
        REQUIRE(sample.join("|"_el) == "red|green|blue|red"_el);

        auto uniqueRandom = CountingRandom{};
        auto unique = uniqueRandom.buildUniqueElementList(ElementCount{2}, choices);
        static_assert(std::is_same_v<decltype(unique), StringViewList>);
        REQUIRE(unique.join("|"_el) == "blue|green"_el);
    }

    void testShuffle() {
        auto random = CountingRandom{};
        auto values = std::vector<int>{1, 2, 3, 4};
        random.shuffle(values);

        auto sorted = values;
        std::ranges::sort(sorted);
        REQUIRE_EQUAL(sorted, (std::vector<int>{1, 2, 3, 4}));

        auto list = List<int>{{1, 2, 3}};
        random.shuffle(list);
        REQUIRE_EQUAL(list.sorted().toStdVector(), (std::vector<int>{1, 2, 3}));

        auto palette = StringViewList{"red"_el, "green"_el, "blue"_el};
        auto paletteRandom = CountingRandom{};
        paletteRandom.shuffle(palette);
        REQUIRE(palette.join("|"_el) == "blue|green|red"_el);
    }
};
