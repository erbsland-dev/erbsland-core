// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/UnsafeU8StringBuffer.hpp>
#include <erbsland/text/impl/UnsafeU8StringEditorAccess.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstring>
#include <string>
#include <string_view>

using el::text::U8StringEditor;
using el::unit::ByteLength;

TESTED_TARGETS(UnsafeU8StringEditorAccess UnsafeU8StringBuffer)
class UnsafeU8StringEditorAccessTest final : public el::UnitTest {
public:
    void testEmptyStringReturnsNullPointer() {
        const auto text = U8StringEditor{};

        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.data(), nullptr);
    }

    void testStringReturnsNullTerminatedData() {
        const auto text = U8StringEditor{std::string_view{"Hello"}};
        const auto *data = el::text::impl::UnsafeU8StringEditorAccess{text}.data();

        REQUIRE_NOT_EQUAL(data, nullptr);
        REQUIRE_EQUAL(std::strcmp(data, "Hello"), 0);
    }

    void testBufferFullDataSizeConstructor() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{std::size_t{6U}};

        REQUIRE_NOT_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{6U});
        REQUIRE_EQUAL(buffer.capacity(), ByteLength{5U});
        REQUIRE_EQUAL(buffer.data()[5U], '\0');
    }

    void testBufferCapacityConstructor() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{5U}};

        REQUIRE_NOT_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{6U});
        REQUIRE_EQUAL(buffer.capacity(), ByteLength{5U});
        REQUIRE_EQUAL(buffer.data()[5U], '\0');
    }

    void testBufferTakeWithFinalLength() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{5U}};
        std::memcpy(buffer.data(), "hello", 5U);

        const auto text = buffer.take(ByteLength{5U});

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"hello"});
        REQUIRE_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{0U});
        REQUIRE_EQUAL(buffer.capacity(), ByteLength::zero());
    }

    void testBufferDefaultTakeUsesWholeCapacity() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{5U}};
        std::memcpy(buffer.data(), "hello", 5U);

        const auto text = buffer.take();

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"hello"});
    }

    void testBufferTakeWithShorterLength() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{5U}};
        std::memcpy(buffer.data(), "hello", 5U);

        const auto text = buffer.take(ByteLength{2U});

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"he"});
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.data()[2U], '\0');
    }

    void testBufferTakeZeroReturnsEmptyString() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{3U}};

        const auto text = buffer.take(ByteLength::zero());

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.data(), nullptr);
        REQUIRE_EQUAL(buffer.data(), nullptr);
    }

    void testBufferMoveLeavesSourceEmpty() {
        auto source = el::text::impl::UnsafeU8StringBuffer{ByteLength{2U}};
        auto moved = std::move(source);

        REQUIRE_EQUAL(source.data(), nullptr);
        REQUIRE_EQUAL(source.dataSize(), std::size_t{0U});
        REQUIRE_EQUAL(moved.capacity(), ByteLength{2U});
    }
};
