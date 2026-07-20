// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unittest/TextHelper.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(Hasher)
class HasherTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void requireDigest(const el::mem::ByteBlock &data, const std::string_view expected) {
        auto hasher = Hasher{HashAlgorithm::Sha3_256};
        hasher.update(data);
        REQUIRE_EQUAL(hasher.finalize(), bytesFromHex(expected));
    }

    void testInvalidPlaceholder() {
        auto hasher = Hasher{};
        REQUIRE_FALSE(hasher.isValid());
        REQUIRE_THROWS_AS(el::err::LogicError, hasher.algorithm());
        REQUIRE_THROWS_AS(el::err::LogicError, hasher.reset());
        REQUIRE_THROWS_AS(el::err::LogicError, hasher.update(std::span<const std::byte>{}));
        REQUIRE_THROWS_AS(el::err::LogicError, hasher.finalize());
    }

    void testInputOverloads() {
        const auto expected = bytesFromHex("36f028580bb02cc8272a9a020f4200e346e276ae664e45ee80745574e2f5ab80");

        const auto standardBytes =
            std::array<std::byte, 4>{std::byte{0x74}, std::byte{0x65}, std::byte{0x73}, std::byte{0x74}};
        auto standardHasher = Hasher{HashAlgorithm::Sha3_256};
        standardHasher.update(standardBytes);
        REQUIRE_EQUAL(standardHasher.finalize(), expected);

        const auto erbslandBytes = std::array<el::mem::Byte, 4>{
            el::mem::Byte{0x74U}, el::mem::Byte{0x65U}, el::mem::Byte{0x73U}, el::mem::Byte{0x74U}};
        auto erbslandHasher = Hasher{HashAlgorithm::Sha3_256};
        erbslandHasher.update(erbslandBytes);
        REQUIRE_EQUAL(erbslandHasher.finalize(), expected);

        auto blockHasher = Hasher{HashAlgorithm::Sha3_256};
        blockHasher.update(bytesFromHex("74657374"));
        REQUIRE_EQUAL(blockHasher.finalize(), expected);

        auto textHasher = Hasher{HashAlgorithm::Sha3_256};
        textHasher.update(el::text::String{"test"_el});
        REQUIRE_EQUAL(textHasher.finalize(), expected);
    }

    void testTextUsesExactUtf8Bytes() {
        auto emptyTextHasher = Hasher{HashAlgorithm::Sha3_256};
        emptyTextHasher.update(el::text::String{});
        auto emptyBytesHasher = Hasher{HashAlgorithm::Sha3_256};
        emptyBytesHasher.update(std::span<const std::byte>{});
        REQUIRE_EQUAL(emptyTextHasher.finalize(), emptyBytesHasher.finalize());

        const auto utf8Text = el::text::String{"Grüezi 😀"_el};
        const auto utf8Bytes = std::string{"Grüezi \xF0\x9F\x98\x80"};
        auto textHasher = Hasher{HashAlgorithm::Sha3_256};
        textHasher.update(utf8Text);
        auto byteHasher = Hasher{HashAlgorithm::Sha3_256};
        byteHasher.update(std::as_bytes(std::span<const char>{utf8Bytes}));
        REQUIRE_EQUAL(textHasher.finalize(), byteHasher.finalize());

        const auto slicedSource = el::text::String{"xxhash meyy"_el};
        const auto slicedText =
            slicedSource.slice(el::unit::ByteRange{el::unit::ByteIndex{2U}, el::unit::ByteIndex{9U}});
        auto slicedHasher = Hasher{HashAlgorithm::Sha3_256};
        slicedHasher.update(slicedText);
        auto slicedBytesHasher = Hasher{HashAlgorithm::Sha3_256};
        const auto expectedSlice = std::string_view{"hash me"};
        slicedBytesHasher.update(std::as_bytes(std::span<const char>{expectedSlice}));
        REQUIRE_EQUAL(slicedHasher.finalize(), slicedBytesHasher.finalize());

        const auto malformedBytes = el::unittest::th::stdStringFromHex("41 C0 42");
        const auto malformedText = el::text::String{std::string_view{malformedBytes}};
        auto malformedTextHasher = Hasher{HashAlgorithm::Sha3_256};
        malformedTextHasher.update(malformedText);
        auto malformedBytesHasher = Hasher{HashAlgorithm::Sha3_256};
        malformedBytesHasher.update(std::as_bytes(std::span<const char>{malformedBytes}));
        REQUIRE_EQUAL(malformedTextHasher.finalize(), malformedBytesHasher.finalize());
    }

    void testLifecycle() {
        auto hasher = Hasher{HashAlgorithm::Sha3_256};
        REQUIRE(hasher.isValid());
        REQUIRE_EQUAL(hasher.algorithm(), HashAlgorithm::Sha3_256);
        hasher.update(el::text::String{"test"_el});
        const auto digest = hasher.finalize();
        REQUIRE_EQUAL(hasher.finalize(), digest);
        REQUIRE_THROWS_AS(el::err::LogicError, hasher.update(el::text::String{"more"_el}));
        hasher.reset();
        hasher.update(el::text::String{"test"_el});
        REQUIRE_EQUAL(hasher.finalize(), digest);
    }

    void testCopyOnWrite() {
        auto first = Hasher{HashAlgorithm::Sha3_256};
        first.update(el::text::String{"prefix-"_el});
        auto second = first;
        first.update(el::text::String{"first"_el});
        second.update(el::text::String{"second"_el});

        auto expectedFirst = Hasher{HashAlgorithm::Sha3_256};
        expectedFirst.update(el::text::String{"prefix-first"_el});
        auto expectedSecond = Hasher{HashAlgorithm::Sha3_256};
        expectedSecond.update(el::text::String{"prefix-second"_el});
        REQUIRE_EQUAL(first.finalize(), expectedFirst.finalize());
        REQUIRE_EQUAL(second.finalize(), expectedSecond.finalize());

        auto finalizedCopy = first;
        REQUIRE_EQUAL(finalizedCopy.finalize(), first.finalize());
        finalizedCopy.reset();
        finalizedCopy.update(el::text::String{"new"_el});
        REQUIRE(finalizedCopy.finalize() != first.finalize());
    }

    void testMoveLeavesInvalidSource() {
        auto source = Hasher{HashAlgorithm::Sha3_256};
        source.update(el::text::String{"test"_el});
        auto destination = std::move(source);
        REQUIRE_FALSE(source.isValid());
        REQUIRE(destination.isValid());
        REQUIRE_EQUAL(
            destination.finalize(), bytesFromHex("36f028580bb02cc8272a9a020f4200e346e276ae664e45ee80745574e2f5ab80"));
    }
};
