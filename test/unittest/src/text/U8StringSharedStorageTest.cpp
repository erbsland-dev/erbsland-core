// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/impl/U8StringData.hpp>
#include <erbsland/text/u8/impl/U8StringLiteralStorage.hpp>
#include <erbsland/text/u8/impl/U8StringReadTools.hpp>
#include <erbsland/text/u8/impl/U8StringSharedStorage.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstring>
#include <span>
#include <string>
#include <string_view>

using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(U8StringSharedStorage)
class U8StringSharedStorageTest final : public el::UnitTest {
public:
    void testDefaultStorageIsEmpty() {
        const auto storage = el::text::impl::U8StringSharedStorage{};

        REQUIRE(storage.isEmpty());
        REQUIRE_EQUAL(storage.data(), nullptr);
        REQUIRE(storage.sharedData().isNull());
        REQUIRE(storage.range().isEmpty());
        REQUIRE(toString(storage.dataView()).empty());
        REQUIRE(toString(storage.dataView(ByteRange::fromSizeT(1U))).empty());
        REQUIRE(storage.capacity().isZero());
        REQUIRE(storage.memoryUsage().isZero());
    }

    void testStorageFromStdStringView() {
        const auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"Hello"}};

        REQUIRE_FALSE(storage.isEmpty());
        REQUIRE_FALSE(storage.sharedData().isNull());
        REQUIRE_EQUAL(storage.sharedData().constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(5U));
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"Hello"});
        REQUIRE_EQUAL(storage.data()[5], '\0');
        REQUIRE_EQUAL(storage.capacity(), ByteLength{5U});
        REQUIRE_EQUAL(storage.memoryUsage(), expectedMemoryUsage(5U));
    }

    void testStorageFromStdU8StringView() {
        const auto storage = el::text::impl::U8StringSharedStorage{std::u8string_view{u8"Hello"}};

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"Hello"});
    }

    void testStorageFromLiteralStorage() {
        const auto literal = el::text::impl::U8StringLiteralStorage{"Hello", 5U};
        const auto storage = el::text::impl::U8StringSharedStorage{literal};

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(storage.data(), literal.data().data());
    }

    void testStorageFromDataViewCopiesSelectedRange() {
        const auto source = el::text::impl::U8StringLiteralStorage{"abcdef", 6U};
        const auto range = ByteRange{ByteIndex{2U}, ByteLength{3U}};
        const auto storage = el::text::impl::U8StringSharedStorage{source.dataView(range)};

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"cde"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(3U));
        REQUIRE_EQUAL(storage.sharedData().constGet()->size(), 4U);
    }

    void testStorageFromBytesCopiesData() {
        const auto text = std::string_view{"Hello"};

        const auto storage =
            el::text::impl::U8StringSharedStorage::fromBytes(std::span<const char>{text.data(), text.size()});

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"Hello"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(5U));
        REQUIRE_EQUAL(storage.data()[5], '\0');
    }

    void testStorageForSizeCreatesWritableData() {
        auto storage = el::text::impl::U8StringSharedStorage::forSize(3U);
        std::memcpy(storage.dataForWrite(), "abc", 3U);
        storage.dataForWrite()[3] = '\0';

        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(3U));
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"abc"});
        REQUIRE_EQUAL(storage.data()[3], '\0');
    }

    void testStorageFromExistingSharedDataAndRange() {
        auto data = el::text::impl::createU8StringData(std::string_view{"abcdef"});
        const auto range = ByteRange{ByteIndex{1U}, ByteLength{3U}};
        const auto storage = el::text::impl::U8StringSharedStorage{data, range};

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"bcd"});
        REQUIRE_EQUAL(storage.range(), range);
        REQUIRE(data.isShared());
    }

    void testClearKeepsReservedCapacity() {
        auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"Hello"}};
        storage.reserve(ByteLength{9U});

        storage.clear();

        REQUIRE(storage.isEmpty());
        REQUIRE_FALSE(storage.sharedData().isNull());
        REQUIRE_NOT_EQUAL(storage.data(), nullptr);
        REQUIRE(storage.range().isEmpty());
        REQUIRE_EQUAL(storage.capacity(), ByteLength{9U});
        REQUIRE_EQUAL(storage.memoryUsage(), expectedMemoryUsage(9U));
    }

    void testDetachEmptyStorageIsNoOp() {
        auto storage = el::text::impl::U8StringSharedStorage{};

        storage.detach();

        REQUIRE(storage.isEmpty());
        REQUIRE(storage.sharedData().isNull());
    }

    void testDetachUniqueFullRangeIsNoOp() {
        auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"Hello"}};
        const auto *oldData = storage.sharedData().constGet();

        storage.detach();

        REQUIRE_EQUAL(storage.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"Hello"});
    }

    void testDetachSharedFullRangeClonesData() {
        auto first = el::text::impl::U8StringSharedStorage{std::string_view{"Hello"}};
        const auto second = first;
        const auto *oldData = first.sharedData().constGet();

        first.detach();

        REQUIRE_NOT_EQUAL(first.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(second.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(first.range(), ByteRange::fromSizeT(5U));
        REQUIRE_EQUAL(toString(first.dataView()), std::string{"Hello"});
        REQUIRE_EQUAL(toString(second.dataView()), std::string{"Hello"});
    }

    void testDetachPartialRangeMaterializesSelectedBytes() {
        auto data = el::text::impl::createU8StringData(std::string_view{"abcdef"});
        const auto range = ByteRange{ByteIndex{2U}, ByteLength{2U}};
        auto storage = el::text::impl::U8StringSharedStorage{data, range};
        const auto *oldData = storage.sharedData().constGet();

        storage.detach();

        REQUIRE_NOT_EQUAL(storage.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(2U));
        REQUIRE_EQUAL(storage.sharedData().constGet()->size(), 3U);
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"cd"});
    }

    void testReserveEmptyStorageCreatesReservedCapacity() {
        auto storage = el::text::impl::U8StringSharedStorage{};

        storage.reserve(ByteLength{8U});

        REQUIRE(storage.isEmpty());
        REQUIRE_FALSE(storage.sharedData().isNull());
        REQUIRE(storage.range().isEmpty());
        REQUIRE_EQUAL(storage.capacity(), ByteLength{8U});
        REQUIRE_EQUAL(storage.memoryUsage(), expectedMemoryUsage(8U));
    }

    void testReservePartialRangeMaterializesSelectedBytes() {
        auto data = el::text::impl::createU8StringData(std::string_view{"abcdef"});
        auto storage = el::text::impl::U8StringSharedStorage{data, ByteRange{ByteIndex{2U}, ByteLength{2U}}};

        storage.reserve(ByteLength{6U});

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"cd"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(2U));
        REQUIRE_EQUAL(storage.capacity(), ByteLength{6U});
        REQUIRE_EQUAL(storage.sharedData().constGet()->size(), 3U);
        REQUIRE_EQUAL(storage.sharedData().constGet()->capacity(), 7U);
        REQUIRE_EQUAL(storage.data()[2], '\0');
    }

    void testEnsureMutableCapacityKeepsUniqueFullRangeWhenLargeEnough() {
        auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"abc"}};
        storage.reserve(ByteLength{8U});
        const auto *oldData = storage.sharedData().constGet();

        storage.ensureMutableCapacity(6U);

        REQUIRE_EQUAL(storage.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"abc"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(3U));
        REQUIRE_EQUAL(storage.capacity(), ByteLength{8U});
        REQUIRE_EQUAL(storage.data()[3], '\0');
    }

    void testEnsureMutableCapacityDetachesSharedStorage() {
        auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"abc"}};
        const auto copy = storage;
        const auto *oldData = storage.sharedData().constGet();

        storage.ensureMutableCapacity(6U);

        REQUIRE_NOT_EQUAL(storage.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(copy.sharedData().constGet(), oldData);
        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"abc"});
        REQUIRE_EQUAL(toString(copy.dataView()), std::string{"abc"});
        REQUIRE_EQUAL(storage.data()[3], '\0');
    }

    void testEnsureMutableCapacityMaterializesPartialRangeAndResizeMaintainsNullTerminator() {
        auto data = el::text::impl::createU8StringData(std::string_view{"abcdef"});
        auto storage = el::text::impl::U8StringSharedStorage{data, ByteRange{ByteIndex{2U}, ByteLength{2U}}};

        storage.ensureMutableCapacity(8U);
        storage.dataForWrite()[2] = '!';
        storage.resize(3U);

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"cd!"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(3U));
        REQUIRE(storage.capacity() >= ByteLength{8U});
        REQUIRE_EQUAL(storage.data()[3], '\0');
    }

    void testShrinkToFitResetsEmptyStorage() {
        auto storage = el::text::impl::U8StringSharedStorage{};
        storage.reserve(ByteLength{8U});

        storage.shrinkToFit();

        REQUIRE(storage.isEmpty());
        REQUIRE(storage.sharedData().isNull());
        REQUIRE(storage.capacity().isZero());
        REQUIRE(storage.memoryUsage().isZero());
    }

    void testShrinkToFitMaterializesExactVisibleSize() {
        auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"Hello"}};
        storage.reserve(ByteLength{9U});

        storage.shrinkToFit();

        REQUIRE_EQUAL(toString(storage.dataView()), std::string{"Hello"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(5U));
        REQUIRE_EQUAL(storage.capacity(), ByteLength{5U});
        REQUIRE_EQUAL(storage.memoryUsage(), expectedMemoryUsage(5U));
    }

    void testDataViewWithCustomRange() {
        const auto storage = el::text::impl::U8StringSharedStorage{std::string_view{"abcdef"}};
        const auto range = ByteRange{ByteIndex{3U}, ByteLength{2U}};

        REQUIRE_EQUAL(toString(storage.dataView(range)), std::string{"de"});
        REQUIRE_EQUAL(storage.range(), ByteRange::fromSizeT(6U));
    }

private:
    [[nodiscard]] static auto expectedMemoryUsage(const std::size_t capacity) -> ByteLength {
        return ByteLength::fromSizeT(sizeof(el::text::impl::U8StringData) + capacity + 1U);
    }

    [[nodiscard]] static auto toString(const el::text::impl::U8StringDataView &view) noexcept -> std::string {
        return el::text::impl::U8StringReadTools{view}.toStdString();
    }
};
