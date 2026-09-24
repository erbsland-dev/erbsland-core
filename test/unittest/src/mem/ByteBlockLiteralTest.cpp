// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlockLiteral.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

using el::mem::Byte;
using el::mem::ByteBlockLiteral;
using el::unit::ByteLength;

TESTED_TARGETS(ByteBlockLiteral unsafeCreateByteBlockLiteral)
class ByteBlockLiteralTest final : public el::UnitTest {
public:
    void testEmptyLiteral() {
        constexpr auto literal = ByteBlockLiteral{};

        static_assert(std::is_trivially_copyable_v<ByteBlockLiteral>);
        static_assert(std::is_nothrow_copy_constructible_v<ByteBlockLiteral>);
        static_assert(std::is_nothrow_move_constructible_v<ByteBlockLiteral>);
        static_assert(literal.isEmpty());
        static_assert(literal.length() == ByteLength{});
        REQUIRE(literal.span().empty());
    }

    void testStaticArraySources() {
        static constexpr std::uint8_t cUInt8Data[]{1U, 2U, 3U};
        static constexpr std::byte cStdByteData[]{std::byte{4U}, std::byte{5U}};
        static constexpr Byte cByteData[]{Byte{6U}, Byte{7U}, Byte{8U}, Byte{9U}};
        constexpr auto uint8Literal = ByteBlockLiteral{cUInt8Data};
        constexpr auto stdByteLiteral = ByteBlockLiteral{cStdByteData};
        constexpr auto byteLiteral = ByteBlockLiteral{cByteData};

        static_assert(uint8Literal.length() == ByteLength{3U});
        static_assert(stdByteLiteral.length() == ByteLength{2U});
        static_assert(byteLiteral.length() == ByteLength{4U});
        REQUIRE_EQUAL(toValues(uint8Literal), std::vector<std::uint8_t>({1U, 2U, 3U}));
        REQUIRE_EQUAL(toValues(stdByteLiteral), std::vector<std::uint8_t>({4U, 5U}));
        REQUIRE_EQUAL(toValues(byteLiteral), std::vector<std::uint8_t>({6U, 7U, 8U, 9U}));
    }

    void testStaticSpanSources() {
        static constexpr auto cUInt8Data = std::array<std::uint8_t, 3U>{10U, 11U, 12U};
        static constexpr auto cStdByteData = std::array<std::byte, 2U>{std::byte{13U}, std::byte{14U}};
        static constexpr auto cByteData = std::array<Byte, 2U>{Byte{15U}, Byte{16U}};
        constexpr auto uint8Literal = ByteBlockLiteral{std::span<const std::uint8_t>{cUInt8Data}.subspan(1U)};
        constexpr auto stdByteLiteral = ByteBlockLiteral{std::span<const std::byte>{cStdByteData}};
        constexpr auto byteLiteral = ByteBlockLiteral{el::mem::ConstByteSpan{cByteData}.first(1U)};

        REQUIRE_EQUAL(toValues(uint8Literal), std::vector<std::uint8_t>({11U, 12U}));
        REQUIRE_EQUAL(toValues(stdByteLiteral), std::vector<std::uint8_t>({13U, 14U}));
        REQUIRE_EQUAL(toValues(byteLiteral), std::vector<std::uint8_t>({15U}));
    }

    void testValueFactoryUsesStaticStorage() {
        constexpr auto literal = ByteBlockLiteral::fromValues<1U, 2U, 4U, 5U>();
        constexpr auto sameLiteral = ByteBlockLiteral::fromValues<1U, 2U, 4U, 5U>();

        static_assert(!literal.isEmpty());
        static_assert(literal.length() == ByteLength{4U});
        REQUIRE_EQUAL(toValues(literal), std::vector<std::uint8_t>({1U, 2U, 4U, 5U}));
        REQUIRE_EQUAL(literal.span().data(), sameLiteral.span().data());
    }

    void testInternalFactorySelectsVisiblePrefix() {
        static constexpr std::uint8_t cData[]{20U, 21U, 22U, 0U};
        const auto literal = el::mem::impl::unsafeCreateByteBlockLiteral(cData, 3U);

        REQUIRE_EQUAL(toValues(literal), std::vector<std::uint8_t>({20U, 21U, 22U}));
        REQUIRE_EQUAL(literal.span().data(), el::mem::toConstByteSpan(std::span{cData}).data());
    }

private:
    [[nodiscard]] static auto toValues(const ByteBlockLiteral literal) -> std::vector<std::uint8_t> {
        auto result = std::vector<std::uint8_t>{};
        result.reserve(literal.span().size());
        for (const auto byte : literal.span()) {
            result.push_back(byte.toUInt8());
        }
        return result;
    }
};
