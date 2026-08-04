// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cryptology/impl/symmetric/UnsafeSymmetricKeyAccess.hpp>
#include <erbsland/cryptology/symmetric/CryptographicDataBlock.hpp>
#include <erbsland/cryptology/symmetric/SymmetricIv.hpp>
#include <erbsland/cryptology/symmetric/SymmetricKey.hpp>
#include <erbsland/cryptology/symmetric/SymmetricNonce.hpp>
#include <erbsland/cryptology/symmetric/SymmetricTag.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <concepts>

using namespace el::cryptology;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::unit::ByteLength;

TESTED_TARGETS(CryptographicDataBlock SymmetricKey SymmetricTag SymmetricIv SymmetricNonce)
class CryptographicDataBlockTest final : public el::UnitTest {
private:
    template <typename T>
    static constexpr bool HasPublicData = requires(const T &value) { value.data(); };

    template <typename T>
    static constexpr bool HasPublicSpan = requires(const T &value) { value.span(); };

    template <typename T>
    static constexpr bool HasPublicToString = requires(const T &value) { value.toString(); };

    static_assert(!HasPublicData<SymmetricKey>);
    static_assert(!HasPublicSpan<SymmetricKey>);
    static_assert(!HasPublicToString<SymmetricKey>);
    static_assert(HasPublicData<SymmetricTag>);
    static_assert(HasPublicSpan<SymmetricTag>);
    static_assert(HasPublicToString<SymmetricTag>);

public:
    void testEmptyValues() {
        const auto key = SymmetricKey{};
        const auto tag = SymmetricTag{};
        const auto iv = SymmetricIv{};
        const auto nonce = SymmetricNonce{};
        REQUIRE(key.isEmpty());
        REQUIRE(tag.isEmpty());
        REQUIRE(iv.isEmpty());
        REQUIRE(nonce.isEmpty());
        REQUIRE(key.byteLength().isZero());
        REQUIRE_EQUAL(key.bitLength(), 0U);
        REQUIRE(tag.toString().isEmpty());
    }

    void testOwningAndBorrowedConstruction() {
        auto source = ByteBlock({0x01U, 0x02U, 0xabU});
        REQUIRE_FALSE(source.isSensitive());
        const auto key = SymmetricKey{source};
        REQUIRE(source.isSensitive());
        REQUIRE_EQUAL(key.byteLength(), ByteLength{3U});
        REQUIRE_EQUAL(key.bitLength(), 24U);
        REQUIRE(std::ranges::equal(el::cryptology::impl::UnsafeSymmetricKeyAccess{key}.span(), source.span()));

        const auto ordinary = ByteBlock({0x10U, 0x20U});
        const auto nonce = SymmetricNonce{ordinary.span()};
        REQUIRE_FALSE(ordinary.isSensitive());
        REQUIRE(nonce.data().isSensitive());
        REQUIRE_EQUAL(nonce.data(), ordinary);
    }

    void testPublicDataAndFormatting() {
        const auto bytes = ByteBlock({0x01U, 0x02U, 0xabU, 0xffU});
        const auto tag = SymmetricTag{bytes};
        const auto iv = SymmetricIv{bytes};
        const auto nonce = SymmetricNonce{bytes};
        REQUIRE(tag.data().isSensitive());
        REQUIRE_EQUAL(tag.data(), bytes);
        REQUIRE(std::ranges::equal(tag.span(), bytes.span()));
        REQUIRE_EQUAL(tag.toString(), "0102abff"_el);
        REQUIRE_EQUAL(iv.toString(), "0102abff"_el);
        REQUIRE_EQUAL(nonce.toString(), "0102abff"_el);
    }
};
