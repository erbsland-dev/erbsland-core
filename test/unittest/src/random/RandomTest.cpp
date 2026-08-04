// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/math/IntegerRange.hpp>
#include <erbsland/mem/impl/ByteBlockData.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/u8/impl/U8StringData.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/ItemCount.hpp>
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
using el::text::String;
using el::text::StringList;
using el::unit::ByteLength;
using el::unit::CpLength;
using el::unit::ItemCount;
using el::util::HashSet;
using el::util::List;
using el::util::Set;
using namespace el::text::literals;

namespace erbsland::test::randomtest {

class CountingRandom : public Random {
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

class SecureCountingRandom final : public CountingRandom {
public:
    [[nodiscard]] auto isSecure() const noexcept -> bool override { return true; }
};

class ThrowingRandom final : public Random {
public:
    [[nodiscard]] auto isSecure() const noexcept -> bool override { return true; }
    [[nodiscard]] auto getInt32(const int32_t minimum, const int32_t maximum) -> int32_t override {
        return static_cast<int32_t>(getInt64(minimum, maximum));
    }
    [[nodiscard]] auto getUInt32(const uint32_t minimum, const uint32_t maximum) -> uint32_t override {
        return static_cast<uint32_t>(getUInt64(minimum, maximum));
    }
    [[nodiscard]] auto getInt64(const int64_t minimum, const int64_t maximum) -> int64_t override {
        return static_cast<int64_t>(getUInt64(static_cast<uint64_t>(minimum), static_cast<uint64_t>(maximum)));
    }
    [[nodiscard]] auto getUInt64(uint64_t, uint64_t) -> uint64_t override {
        if (_selectionCount++ != 0U) {
            throw std::runtime_error{"Injected random selection failure."};
        }
        return 0U;
    }
    [[nodiscard]] auto getDouble(double, double) -> double override {
        throw std::runtime_error{"Injected random selection failure."};
    }
    [[nodiscard]] auto getBool() -> bool override { throw std::runtime_error{"Injected random selection failure."}; }
    void fillBytes(const std::span<std::byte> destination) override {
        if (!destination.empty()) {
            destination.front() = std::byte{0x5a};
        }
        throw std::runtime_error{"Injected random byte failure."};
    }

private:
    std::size_t _selectionCount{};
};

struct RandomEraseEvent final {
    std::size_t size{};
    bool isZero{};
};

std::vector<RandomEraseEvent> gRandomEraseEvents;

void observeRandomErase(const std::span<const std::byte> bytes) noexcept {
    gRandomEraseEvents.push_back({bytes.size(), std::ranges::all_of(bytes, [](const std::byte value) noexcept -> bool {
                                      return value == std::byte{};
                                  })});
}

class RandomEraseObserverGuard final {
public:
    RandomEraseObserverGuard() {
        gRandomEraseEvents.clear();
        el::mem::impl::setSecureEraseObserver(observeRandomErase);
    }
    ~RandomEraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
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

        const auto list = random.buildIntegerList(ItemCount{4}, 3, 5);
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{3, 4, 5, 3}));
    }

    void testEmptyInputs() {
        auto random = CountingRandom{};

        REQUIRE(random.selectIndex(ItemCount::zero()).isNoIndex());
        REQUIRE(random.selectIndex(ItemCount::infinite()).isNoIndex());
        REQUIRE_EQUAL(random.selectElement(std::vector<int>{}, 99), 99);
        REQUIRE(random.buildIntegerList(ItemCount::zero(), 1, 3).count().isZero());
        REQUIRE(random.buildString(CpLength{4}, CharSet{}).isEmpty());
        REQUIRE(random.buildString(CpLength::zero(), CharSet::fromPattern("A-Z"_el)).isEmpty());
        REQUIRE(random.buildByteBlock(ByteLength::zero()).isEmpty());
        REQUIRE(random.buildByteBuffer(ByteLength::zero()).isEmpty());

        auto secureRandom = SecureCountingRandom{};
        const auto emptySecureBuffer = secureRandom.buildByteBuffer(ByteLength::zero());
        REQUIRE(emptySecureBuffer.isEmpty());
        REQUIRE(emptySecureBuffer.isSensitive());
    }

    void testStringAndBytes() {
        auto random = CountingRandom{};

        const auto text = random.buildString(CpLength{3}, CharSet::fromPattern("A-C"_el));
        REQUIRE_EQUAL(text, "ABC"_el);

        const auto block = random.buildByteBlock(ByteLength{4});
        REQUIRE_EQUAL(block.length(), ByteLength{4});
        REQUIRE_FALSE(block.isSensitive());
        REQUIRE_FALSE(random.isSecure());
        REQUIRE_EQUAL(block.toUInt8Vector(), (std::vector<uint8_t>{3U, 4U, 5U, 6U}));

        const auto buffer = random.buildByteBuffer(ByteLength{4});
        REQUIRE_EQUAL(buffer.length(), ByteLength{4});
        REQUIRE_EQUAL(buffer.toUInt8Vector(), (std::vector<uint8_t>{7U, 8U, 9U, 10U}));
    }

    void testStringRejectsUnrepresentableCapacity() {
        auto random = CountingRandom{};

        REQUIRE_THROWS_AS(el::err::OutOfRangeError, random.buildString(CpLength::maximum(), CharSet{"🦊"_el}));
    }

    void testSecureStringAndBytesUseProtectedAllocations() {
        const auto guard = RandomEraseObserverGuard{};
        {
            auto random = SecureCountingRandom{};
            const auto sensitiveText = random.buildString(CpLength{3U}, CharSet{"Aé🦊"_el});
            REQUIRE(sensitiveText.isSensitive());
            REQUIRE_EQUAL(sensitiveText.characterLength(), CpLength{3U});
            REQUIRE_EQUAL(sensitiveText.charAt(el::unit::CpIndex{0U}), el::text::Char{U'A'});
            REQUIRE_EQUAL(sensitiveText.charAt(el::unit::CpIndex{1U}), el::text::Char{U'é'});
            REQUIRE_EQUAL(sensitiveText.charAt(el::unit::CpIndex{2U}), el::text::Char{U'🦊'});
            REQUIRE(gRandomEraseEvents.empty());
        }
        REQUIRE_EQUAL(gRandomEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(
            gRandomEraseEvents.front().size, el::text::impl::U8StringData::allocationSizeForCapacity(std::size_t{13U}));
        REQUIRE(gRandomEraseEvents.front().isZero);

        gRandomEraseEvents.clear();
        {
            auto random = SecureCountingRandom{};
            REQUIRE(random.isSecure());
            const auto sensitiveBytes = random.buildByteBlock(ByteLength{4U});
            REQUIRE_EQUAL(sensitiveBytes.length(), ByteLength{4U});
            REQUIRE(sensitiveBytes.isSensitive());
            REQUIRE_EQUAL(sensitiveBytes.get(el::unit::ByteIndex{0U}), el::mem::Byte{0U});
            REQUIRE_EQUAL(sensitiveBytes.get(el::unit::ByteIndex{3U}), el::mem::Byte{3U});
            REQUIRE(gRandomEraseEvents.empty());
        }
        REQUIRE_EQUAL(gRandomEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(
            gRandomEraseEvents.front().size, el::mem::impl::ByteBlockData::allocationSizeForCapacity(std::size_t{4U}));
        REQUIRE(gRandomEraseEvents.front().isZero);

        gRandomEraseEvents.clear();
        {
            auto random = SecureCountingRandom{};
            const auto sensitiveBytes = random.buildByteBuffer(ByteLength{4U});
            REQUIRE(sensitiveBytes.isSensitive());
            REQUIRE_EQUAL(sensitiveBytes.length(), ByteLength{4U});
        }
        REQUIRE_FALSE(gRandomEraseEvents.empty());
    }

    void testSensitiveBuildersErasePartialDataAfterGeneratorFailure() {
        const auto guard = RandomEraseObserverGuard{};
        {
            auto random = ThrowingRandom{};
            REQUIRE_THROWS(random.buildString(CpLength{3U}, CharSet::fromPattern("A-C"_el)));
        }
        REQUIRE_EQUAL(gRandomEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(
            gRandomEraseEvents.front().size, el::text::impl::U8StringData::allocationSizeForCapacity(std::size_t{4U}));
        REQUIRE(gRandomEraseEvents.front().isZero);

        gRandomEraseEvents.clear();
        {
            auto random = ThrowingRandom{};
            REQUIRE_THROWS(random.buildByteBlock(ByteLength{4U}));
        }
        REQUIRE_EQUAL(gRandomEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(
            gRandomEraseEvents.front().size, el::mem::impl::ByteBlockData::allocationSizeForCapacity(std::size_t{4U}));
        REQUIRE(gRandomEraseEvents.front().isZero);
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
        REQUIRE_EQUAL(listRandom.buildElementList(ItemCount{4}, choices).toStdVector(), (std::vector<int>{1, 2, 3, 1}));
        REQUIRE_EQUAL(random.buildElementList(ItemCount{2}, hashSetChoices).count(), ItemCount{2});

        const auto unique = random.buildUniqueElementList(ItemCount{2}, choices);
        REQUIRE_EQUAL(unique.count(), ItemCount{2});
        const auto uniqueSet = unique.toStdSet();
        REQUIRE_EQUAL(uniqueSet.size(), 2U);
        REQUIRE_EQUAL(random.buildUniqueElementList(ItemCount{2}, hashSetChoices).count(), ItemCount{2});
    }

    void testDerivedListElementSelection() {
        auto choices = StringList{"red"_el, "green"_el, "blue"_el};

        auto random = CountingRandom{};
        const auto selected = random.selectElement(choices);
        const auto defaultSelected = random.selectElement(StringList{}, String{"white"_el});
        REQUIRE_EQUAL(selected, "red"_el);
        REQUIRE_EQUAL(defaultSelected, "white"_el);

        auto listRandom = CountingRandom{};
        auto sample = listRandom.buildElementList(ItemCount{4}, choices);
        static_assert(std::is_same_v<decltype(sample), StringList>);
        const auto sampleText = sample.join("|"_el);
        REQUIRE_EQUAL(sampleText, "red|green|blue|red"_el);

        auto uniqueRandom = CountingRandom{};
        auto unique = uniqueRandom.buildUniqueElementList(ItemCount{2}, choices);
        static_assert(std::is_same_v<decltype(unique), StringList>);
        const auto uniqueText = unique.join("|"_el);
        REQUIRE_EQUAL(uniqueText, "blue|green"_el);
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

        auto palette = StringList{"red"_el, "green"_el, "blue"_el};
        auto paletteRandom = CountingRandom{};
        paletteRandom.shuffle(palette);
        const auto paletteText = palette.join("|"_el);
        REQUIRE_EQUAL(paletteText, "blue|green|red"_el);
    }
};
