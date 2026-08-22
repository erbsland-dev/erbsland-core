// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/PlatformU8StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU8StringBuffer.hpp>
#include <erbsland/text/impl/UnsafeU8StringEditorAccess.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstring>
#include <string>
#include <string_view>

using el::text::StringEditor;
using el::unit::ByteLength;
using namespace el::text::literals;

TESTED_TARGETS(UnsafeU8StringEditorAccess UnsafeU8StringBuffer)
class UnsafeU8StringEditorAccessTest final : public el::UnitTest {
public:
    void testEmptyStringReturnsNullPointer() {
        const auto text = StringEditor{};

        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.dataSpan().data(), nullptr);
    }

    void testStringReturnsBoundedDataSpan() {
        const auto text = StringEditor{"Hello"_el};
        const auto data = el::text::impl::UnsafeU8StringEditorAccess{text}.dataSpan();
        const auto textView = std::string_view{data.data(), data.size()};

        REQUIRE_EQUAL(data.size(), 5U);
        REQUIRE_EQUAL(textView, std::string_view{"Hello"});
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
        REQUIRE_EQUAL(el::text::impl::PlatformU8StringAccess{text}.nullTerminatedCharPtr()[2U], '\0');
    }

    void testBufferTakeReadOnlyString() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{5U}};
        std::memcpy(buffer.data(), "hello", 5U);
        const auto *storage = buffer.data();

        const auto text = buffer.takeString();

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"hello"});
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.dataSpan().data(), storage);
        REQUIRE_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.capacity(), ByteLength::zero());
    }

    void testBufferTakeZeroReturnsEmptyString() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteLength{3U}};

        const auto text = buffer.take(ByteLength::zero());

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.dataSpan().data(), nullptr);
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
