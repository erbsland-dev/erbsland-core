// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/UnsafeU16StringBuffer.hpp>
#include <erbsland/text/impl/UnsafeU16StringEditorAccess.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/unit/U16DataLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstring>
#include <string>
#include <string_view>

using el::text::U16StringEditor;
using el::unit::U16DataLength;

TESTED_TARGETS(UnsafeU16StringEditorAccess UnsafeU16StringBuffer)
class UnsafeU16StringEditorAccessTest final : public el::UnitTest {
public:
    void testEmptyStringReturnsNullPointer() {
        const auto text = U16StringEditor{};

        REQUIRE_EQUAL(el::text::impl::UnsafeU16StringEditorAccess{text}.data(), nullptr);
    }

    void testStringReturnsNullTerminatedData() {
        const auto text = U16StringEditor{std::u16string_view{u"Hello"}};
        const auto *data = el::text::impl::UnsafeU16StringEditorAccess{text}.data();

        REQUIRE_NOT_EQUAL(data, nullptr);
        REQUIRE_EQUAL(data[0], u'H');
        REQUIRE_EQUAL(data[4], u'o');
        REQUIRE_EQUAL(data[5], u'\0');
    }

    void testBufferFullDataSizeConstructor() {
        auto buffer = el::text::impl::UnsafeU16StringBuffer{std::size_t{6U}};

        REQUIRE_NOT_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{6U});
        REQUIRE_EQUAL(buffer.capacity(), U16DataLength{5U});
        REQUIRE_EQUAL(buffer.data()[5U], u'\0');
    }

    void testBufferCapacityConstructor() {
        auto buffer = el::text::impl::UnsafeU16StringBuffer{U16DataLength{5U}};

        REQUIRE_NOT_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{6U});
        REQUIRE_EQUAL(buffer.capacity(), U16DataLength{5U});
        REQUIRE_EQUAL(buffer.data()[5U], u'\0');
    }

    void testBufferTakeWithFinalLength() {
        auto buffer = el::text::impl::UnsafeU16StringBuffer{U16DataLength{5U}};
        std::memcpy(buffer.data(), u"hello", 5U * sizeof(char16_t));

        const auto text = buffer.take(U16DataLength{5U});

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdU16String(), std::u16string{u"hello"});
        REQUIRE_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{0U});
        REQUIRE_EQUAL(buffer.capacity(), U16DataLength::zero());
    }

    void testBufferDefaultTakeUsesWholeCapacity() {
        auto buffer = el::text::impl::UnsafeU16StringBuffer{U16DataLength{5U}};
        std::memcpy(buffer.data(), u"hello", 5U * sizeof(char16_t));

        const auto text = buffer.take();

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdU16String(), std::u16string{u"hello"});
    }

    void testBufferTakeWithShorterLength() {
        auto buffer = el::text::impl::UnsafeU16StringBuffer{U16DataLength{5U}};
        std::memcpy(buffer.data(), u"hello", 5U * sizeof(char16_t));

        const auto text = buffer.take(U16DataLength{2U});

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdU16String(), std::u16string{u"he"});
        REQUIRE_EQUAL(el::text::impl::UnsafeU16StringEditorAccess{text}.data()[2U], u'\0');
    }

    void testBufferTakeZeroReturnsEmptyString() {
        auto buffer = el::text::impl::UnsafeU16StringBuffer{U16DataLength{3U}};

        const auto text = buffer.take(U16DataLength::zero());

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(el::text::impl::UnsafeU16StringEditorAccess{text}.data(), nullptr);
        REQUIRE_EQUAL(buffer.data(), nullptr);
    }

    void testBufferMoveLeavesSourceEmpty() {
        auto source = el::text::impl::UnsafeU16StringBuffer{U16DataLength{2U}};
        auto moved = std::move(source);

        REQUIRE_EQUAL(source.data(), nullptr);
        REQUIRE_EQUAL(source.dataSize(), std::size_t{0U});
        REQUIRE_EQUAL(moved.capacity(), U16DataLength{2U});
    }
};
