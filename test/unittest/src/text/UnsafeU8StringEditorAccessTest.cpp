// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Byte.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/text/impl/PlatformU8StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU8StringBuffer.hpp>
#include <erbsland/text/impl/UnsafeU8StringEditorAccess.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

using el::text::StringEditor;
using el::unit::ByteCount;
using namespace el::text::literals;

TESTED_TARGETS(UnsafeU8StringEditorAccess UnsafeU8StringBuffer U8StringAppendTools)
class UnsafeU8StringEditorAccessTest final : public el::UnitTest {
private:
    template <typename T>
    void verifyRawAppend() {
        using Access = el::text::impl::UnsafeU8StringEditorAccess;
        const auto source = std::array<T, 6U>{
            static_cast<T>(0xefU),
            static_cast<T>(0xbbU),
            static_cast<T>(0xbfU),
            static_cast<T>('a'),
            static_cast<T>(0),
            static_cast<T>(0xffU)};
        auto text = StringEditor{"prefix"_el};
        text.markAsSensitive();
        text.reserve(ByteCount{64U});
        const auto *storage = Access{text}.dataSpan().data();
        const auto count = Access::appendTools(text).append(std::span{source});
        REQUIRE_EQUAL(count, el::unit::CpCount{4U});
        REQUIRE_EQUAL(Access{text}.dataSpan().data(), storage);
        REQUIRE(text.isSensitive());
        const auto raw = Access{text}.dataSpan();
        REQUIRE_EQUAL(
            (std::string_view{raw.data(), raw.size()}), el::unittest::th::stdStringFromHex("707265666978efbbbf6100ff"));
        REQUIRE_EQUAL(
            el::text::impl::PlatformU8StringAccess{el::text::String{text}}.nullTerminatedCharPtr()[12U], '\0');
        const auto shared = text;
        REQUIRE_EQUAL(Access::appendTools(text).append(std::span<const T>{}), el::unit::CpCount::zero());
        REQUIRE_EQUAL(Access{text}.dataSpan().data(), storage);
        Access::appendTools(text).append(std::span{source});
        REQUIRE_EQUAL(shared.length(), ByteCount{12U});
        REQUIRE_EQUAL(text.length(), ByteCount{18U});
        REQUIRE_NOT_EQUAL(Access{text}.dataSpan().data(), storage);
        REQUIRE_EQUAL(Access{shared}.dataSpan().data(), storage);
        REQUIRE(text.isSensitive());
    }

    template <typename T>
    void verifyBufferFromSpan() {
        using Buffer = el::text::impl::UnsafeU8StringBuffer;
        static_assert(std::is_constructible_v<Buffer, std::span<const T>>);
        static_assert(std::is_constructible_v<Buffer, std::span<T>>);
        static_assert(std::is_constructible_v<Buffer, std::span<const T, 6U>>);
        static_assert(!std::is_convertible_v<std::span<const T>, Buffer>);
        static_assert(!std::is_constructible_v<el::text::String, std::span<const T>>);

        for (const auto sensitive : {false, true}) {
            auto source = std::array<T, 8U>{
                static_cast<T>('x'),
                static_cast<T>(0xefU),
                static_cast<T>(0xbbU),
                static_cast<T>(0xbfU),
                static_cast<T>('a'),
                static_cast<T>(0),
                static_cast<T>(0xffU),
                static_cast<T>('y')};
            auto buffer = Buffer{std::span<const T>{source}.template subspan<1U, 6U>(), sensitive};
            REQUIRE_EQUAL(buffer.capacity(), ByteCount{6U});
            REQUIRE_EQUAL(buffer.dataSize(), std::size_t{7U});
            REQUIRE_EQUAL(buffer.data()[6U], '\0');
            REQUIRE_EQUAL(buffer.isSensitive(), sensitive);
            const auto *storage = buffer.data();
            source.fill(static_cast<T>(0));

            const auto text = buffer.takeString();
            REQUIRE_EQUAL(text.length(), ByteCount{6U});
            REQUIRE_EQUAL(text.isSensitive(), sensitive);
            REQUIRE_FALSE(text.isEncodingValid());
            const auto raw = el::text::impl::UnsafeU8StringAccess{text}.dataSpan();
            REQUIRE_EQUAL(
                (std::string_view{raw.data(), raw.size()}), el::unittest::th::stdStringFromHex("efbbbf6100ff"));
            REQUIRE_EQUAL(raw.data(), storage);
            REQUIRE_EQUAL(el::text::impl::PlatformU8StringAccess{text}.nullTerminatedCharPtr()[6U], '\0');
            REQUIRE_EQUAL(buffer.data(), nullptr);
        }
    }

    template <typename T>
    void verifyBufferFromEmptySpan() {
        for (const auto sensitive : {false, true}) {
            auto buffer = el::text::impl::UnsafeU8StringBuffer{std::span<const T>{}, sensitive};
            REQUIRE_EQUAL(buffer.capacity(), ByteCount::zero());
            REQUIRE_EQUAL(buffer.isSensitive(), sensitive);
            const auto text = buffer.take();
            REQUIRE(text.isEmpty());
            REQUIRE_EQUAL(text.isSensitive(), sensitive);
            REQUIRE_EQUAL(buffer.data(), nullptr);
        }
    }

public:
    void testRawAppendPreservesBytesAndReusesCapacity() {
        WITH_CONTEXT(verifyRawAppend<char>());
        WITH_CONTEXT(verifyRawAppend<std::byte>());
        WITH_CONTEXT(verifyRawAppend<el::mem::Byte>());
    }

    void testRawAppendFromIndependentStorageGrowsAndMaterializesSlices() {
        using Access = el::text::impl::UnsafeU8StringEditorAccess;
        const auto source = std::array{'x', 'y', 'z'};
        auto text = StringEditor{"abc"_el};
        text.shrinkToFit();
        REQUIRE_EQUAL(Access::appendTools(text).append(std::span{source}), el::unit::CpCount{3U});
        REQUIRE_EQUAL(text, "abcxyz"_el);

        const auto slice = el::text::String{"prefix-value-suffix"_el}.slice({el::unit::ByteIndex{7U}, ByteCount{5U}});
        auto editor = StringEditor{slice};
        Access::appendTools(editor).append(std::span{source});
        REQUIRE_EQUAL(editor, "valuexyz"_el);
        REQUIRE_EQUAL(slice, "value"_el);
    }

    void testBufferCopiesCharacterSpan() { WITH_CONTEXT(verifyBufferFromSpan<char>()); }

    void testBufferCopiesStandardByteSpan() { WITH_CONTEXT(verifyBufferFromSpan<std::byte>()); }

    void testBufferCopiesCoreByteSpan() { WITH_CONTEXT(verifyBufferFromSpan<el::mem::Byte>()); }

    void testBufferCopiesEmptySpans() {
        WITH_CONTEXT(verifyBufferFromEmptySpan<char>());
        WITH_CONTEXT(verifyBufferFromEmptySpan<std::byte>());
        WITH_CONTEXT(verifyBufferFromEmptySpan<el::mem::Byte>());
    }

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
        REQUIRE_EQUAL(buffer.capacity(), ByteCount{5U});
        REQUIRE_EQUAL(buffer.data()[5U], '\0');
    }

    void testBufferCapacityConstructor() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteCount{5U}};

        REQUIRE_NOT_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{6U});
        REQUIRE_EQUAL(buffer.capacity(), ByteCount{5U});
        REQUIRE_EQUAL(buffer.data()[5U], '\0');
    }

    void testBufferTakeWithFinalLength() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteCount{5U}};
        std::memcpy(buffer.data(), "hello", 5U);

        const auto text = buffer.take(ByteCount{5U});

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"hello"});
        REQUIRE_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.dataSize(), std::size_t{0U});
        REQUIRE_EQUAL(buffer.capacity(), ByteCount::zero());
    }

    void testBufferDefaultTakeUsesWholeCapacity() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteCount{5U}};
        std::memcpy(buffer.data(), "hello", 5U);

        const auto text = buffer.take();

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"hello"});
    }

    void testBufferTakeWithShorterLength() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteCount{5U}};
        std::memcpy(buffer.data(), "hello", 5U);

        const auto text = buffer.take(ByteCount{2U});

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"he"});
        REQUIRE_EQUAL(el::text::impl::PlatformU8StringAccess{text}.nullTerminatedCharPtr()[2U], '\0');
    }

    void testBufferTakeReadOnlyString() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteCount{5U}};
        std::memcpy(buffer.data(), "hello", 5U);
        const auto *storage = buffer.data();

        const auto text = buffer.takeString();

        REQUIRE_EQUAL(el::text::StringConverter{text}.toStdString(), std::string{"hello"});
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.dataSpan().data(), storage);
        REQUIRE_EQUAL(buffer.data(), nullptr);
        REQUIRE_EQUAL(buffer.capacity(), ByteCount::zero());
    }

    void testBufferTakeZeroReturnsEmptyString() {
        auto buffer = el::text::impl::UnsafeU8StringBuffer{ByteCount{3U}};

        const auto text = buffer.take(ByteCount::zero());

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.dataSpan().data(), nullptr);
        REQUIRE_EQUAL(buffer.data(), nullptr);
    }

    void testBufferMoveLeavesSourceEmpty() {
        auto source = el::text::impl::UnsafeU8StringBuffer{ByteCount{2U}};
        auto moved = std::move(source);

        REQUIRE_EQUAL(source.data(), nullptr);
        REQUIRE_EQUAL(source.dataSize(), std::size_t{0U});
        REQUIRE_EQUAL(moved.capacity(), ByteCount{2U});
    }
};
