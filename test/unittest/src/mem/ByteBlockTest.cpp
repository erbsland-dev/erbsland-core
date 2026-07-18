// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/impl/BestGrowth.hpp>
#include <erbsland/mem/impl/ByteBlockData.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <vector>

using el::mem::Byte;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(Byte ByteBlock ByteBlockEditor)
class ByteBlockTest final : public el::UnitTest {
public:
    void testByteHelpers() {
        const auto byte = Byte{0b10101100U};

        REQUIRE_EQUAL(byte.toRawValue(), uint8_t{0b10101100U});
        REQUIRE_EQUAL(byte.toUInt8(), uint8_t{0b10101100U});
        REQUIRE_EQUAL(byte.masked(0b11110000U), uint8_t{0b10100000U});
        REQUIRE(byte.matches(0b11100000U, 0b10100000U));
        REQUIRE_FALSE(byte.matches(0b11110000U, 0b11110000U));
    }

    void testConstructionAndConversion() {
        const auto empty = ByteBlock{};
        REQUIRE(empty.isEmpty());
        REQUIRE(empty.length().isZero());

        const auto filled = ByteBlock{ByteLength{3U}, Byte{0xabu}};
        REQUIRE_EQUAL(filled.toUInt8Vector(), std::vector<uint8_t>({0xabU, 0xabU, 0xabU}));

        const auto fromBytes = ByteBlock{std::vector<Byte>{Byte{1U}, Byte{2U}}};
        REQUIRE_EQUAL(fromBytes.toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        const auto fromUInt8 = ByteBlock{std::vector<uint8_t>{3U, 4U}};
        REQUIRE_EQUAL(fromUInt8.toByteVector(), std::vector<Byte>({Byte{3U}, Byte{4U}}));

        const auto fromChar = ByteBlock{std::vector<char>{'\x05', '\x06'}};
        REQUIRE_EQUAL(fromChar.toCharVector(), std::vector<char>({'\x05', '\x06'}));
    }

    void testMemoryManagementAndCow() {
        auto editor = makeEditor({1U, 2U, 3U});
        editor.reserve(ByteLength{10U});
        const auto reservedCapacity = el::mem::impl::bestGrowthCapacity<el::mem::impl::ByteBlockData>(3U, 10U);
        REQUIRE_EQUAL(editor.capacity(), ByteLength::fromSizeT(reservedCapacity));

        auto copy = editor;
        editor.set(ByteIndex{0U}, Byte{9U});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 2U, 3U}));
        REQUIRE_EQUAL(copy.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));

        editor.clear();
        REQUIRE(editor.isEmpty());
        REQUIRE_EQUAL(editor.capacity(), ByteLength::fromSizeT(reservedCapacity));

        editor.reset();
        REQUIRE(editor.isEmpty());
        REQUIRE(editor.capacity().isZero());

        copy.shrinkToFit();
        REQUIRE_EQUAL(copy.capacity(), copy.length());
    }

    void testGetSetAndThrowingAccess() {
        auto editor = makeEditor({1U, 2U, 3U});

        REQUIRE_EQUAL(editor.get(ByteIndex{1U}), Byte{2U});
        REQUIRE_EQUAL(editor.get(ByteIndex{9U}, Byte{7U}), Byte{7U});
        REQUIRE_EQUAL(editor.get(ByteIndex::noIndex(), Byte{8U}), Byte{8U});
        REQUIRE_THROWS(editor.getOrThrow(ByteIndex{9U}));

        editor.set(ByteIndex{1U}, Byte{5U});
        editor.set(ByteIndex{9U}, Byte{6U});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 3U}));
        REQUIRE_THROWS(editor.setOrThrow(ByteIndex::noIndex(), Byte{1U}));
    }

    void testSliceAndFind() {
        const auto block = makeBlock({1U, 2U, 3U, 2U, 3U, 4U});
        const auto needle = makeBlock({2U, 3U});
        const auto empty = ByteBlock{};

        REQUIRE_EQUAL(
            block.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}}).toUInt8Vector(), std::vector<uint8_t>({2U, 3U, 2U}));
        REQUIRE(block.slice(ByteRange::noRange()).isEmpty());
        REQUIRE(block.startsWith(makeBlock({1U, 2U})));
        REQUIRE(block.endsWith(makeBlock({3U, 4U})));
        REQUIRE(block.contains(needle));
        REQUIRE(block.contains(empty));
        REQUIRE_EQUAL(block.find(needle), ByteIndex{1U});
        REQUIRE_EQUAL(block.find(needle, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(block.findLast(needle), ByteIndex{3U});
        REQUIRE_EQUAL(block.find(empty), ByteIndex::zero());
        REQUIRE_EQUAL(block.findLast(empty), block.endIndex());
        REQUIRE(block.find(makeBlock({9U})).isNoIndex());
    }

    void testModification() {
        auto editor = makeEditor({1U, 2U, 3U, 4U});

        editor.remove(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 4U}));

        editor.insert(ByteIndex{1U}, makeBlock({7U, 8U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 7U, 8U, 4U}));

        editor.replace(ByteRange{ByteIndex{1U}, ByteLength{2U}}, makeBlock({5U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 4U}));

        editor.replace(ByteRange::noRange(), makeBlock({6U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 4U}));

        editor.append(Byte{9U}).append(makeBlock({10U, 11U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 4U, 9U, 10U, 11U}));

        editor.keep(ByteRange{ByteIndex{1U}, ByteLength{3U}});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({5U, 4U, 9U}));

        editor.keep(ByteRange::noRange());
        REQUIRE(editor.isEmpty());
    }

    void testCopyVariantsJoinAndComparison() {
        const auto editor = makeEditor({1U, 2U, 3U, 4U});
        const auto removed = editor.removed(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        const auto replaced = editor.replaced(ByteRange{ByteIndex{1U}, ByteLength{2U}}, makeBlock({9U}));

        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 4U}));
        REQUIRE_EQUAL(removed.toUInt8Vector(), std::vector<uint8_t>({1U, 4U}));
        REQUIRE_EQUAL(replaced.toUInt8Vector(), std::vector<uint8_t>({1U, 9U, 4U}));

        const auto separator = makeEditor({0U});
        const auto joined = separator.join({makeBlock({1U}), makeBlock({2U}), makeBlock({3U})});
        REQUIRE_EQUAL(joined.toUInt8Vector(), std::vector<uint8_t>({1U, 0U, 2U, 0U, 3U}));

        const auto concatenated = ByteBlockEditor::fromJoined({makeBlock({1U}), makeBlock({2U, 3U})});
        REQUIRE_EQUAL(concatenated.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));

        REQUIRE(makeBlock({1U, 2U}) < makeBlock({1U, 3U}));
        REQUIRE(makeEditor({1U, 2U}) == makeBlock({1U, 2U}));
    }

    void testEditorToReadOnlySnapshot() {
        auto editor = makeEditor({1U, 2U, 3U});
        const auto block = ByteBlock{editor};

        editor.set(ByteIndex{0U}, Byte{9U});

        REQUIRE_EQUAL(block.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 2U, 3U}));
    }

    void testEditorCopiesVisibleSlice() {
        const auto block = makeBlock({1U, 2U, 3U, 4U});
        const auto slice = block.slice(ByteIndex{1U}, ByteLength{2U});
        auto editor = ByteBlockEditor{slice};

        editor.set(ByteIndex{0U}, Byte{9U});

        REQUIRE_EQUAL(block.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 4U}));
        REQUIRE_EQUAL(slice.toUInt8Vector(), std::vector<uint8_t>({2U, 3U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 3U}));
    }

private:
    [[nodiscard]] static auto makeBlock(std::initializer_list<uint8_t> bytes) -> ByteBlock {
        return ByteBlock{std::vector<uint8_t>{bytes}};
    }
    [[nodiscard]] static auto makeEditor(std::initializer_list<uint8_t> bytes) -> ByteBlockEditor {
        return ByteBlockEditor{std::vector<uint8_t>{bytes}};
    }
};
