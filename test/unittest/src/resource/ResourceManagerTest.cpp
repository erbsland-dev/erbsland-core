// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/resource/ResourceError.hpp>
#include <erbsland/resource/ResourceManager.hpp>
#include <erbsland/resource/ResourceStorageRegistration.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringBomMode.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/StringEncoding.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <thread>
#include <vector>

namespace erbsland::test::resourcemanagertest {

using namespace text::literals;

const auto cPlainData = std::array<std::uint8_t, 6U>{'h', 'e', 'l', 'l', 'o', 0U};
const auto cEmptyData = std::array<std::uint8_t, 1U>{0U};
const auto cCompressedData =
    std::array<std::uint8_t, 14U>{0x40U, 'a', 'b', 'c', 'd', 0x04U, 0x00U, 0x50U, 'a', 'b', 'c', 'd', 'e', 0U};
const auto cInvalidData = std::array<std::uint8_t, 3U>{0U, 0U, 0U};

class ResourceFixture final {
public:
    [[nodiscard]] static auto plainData() noexcept -> std::span<const std::uint8_t> {
        return std::span{cPlainData}.first(5U);
    }
    [[nodiscard]] static auto emptyData() noexcept -> std::span<const std::uint8_t> {
        return std::span{cEmptyData}.first(0U);
    }
    [[nodiscard]] static auto compressedData() noexcept -> std::span<const std::uint8_t> {
        return std::span{cCompressedData}.first(13U);
    }
    [[nodiscard]] static auto invalidData() noexcept -> std::span<const std::uint8_t> {
        return std::span{cInvalidData}.first(2U);
    }
    [[nodiscard]] static auto createInfo(
        const text::String &path,
        const unit::ByteLength originalSize,
        const unit::ByteLength storedSize,
        const bool compressed,
        const bool hashed) -> std::vector<std::uint8_t> {
        const auto identifier = text::String{"test"_el};
        const auto identifierBytes =
            text::StringEncoder{identifier}.encode(text::StringEncoding::Utf8, text::StringBomMode::Reject);
        const auto pathBytes =
            text::StringEncoder{path}.encode(text::StringEncoding::Utf8, text::StringBomMode::Reject);
        const auto hash = hashed ? mem::ByteBlock{unit::ByteLength{32U}, mem::Byte{0x42U}} : mem::ByteBlock{};
        auto writer = mem::ByteWriter{};
        writer.writeByte(mem::Byte::fromChar('E'))
            .writeByte(mem::Byte::fromChar('L'))
            .writeByte(mem::Byte::fromChar('R'))
            .writeByte(mem::Byte::fromChar('I'))
            .writeUInt8(1U)
            .writeUInt8(compressed ? 1U : 0U)
            .writeUInt8(hashed ? 1U : 0U)
            .writeUInt8(0U)
            .writeUInt64(originalSize.toRawValue())
            .writeUInt64(storedSize.toRawValue())
            .writeUInt32(static_cast<std::uint32_t>(identifierBytes.length().toRawValue()))
            .writeUInt32(static_cast<std::uint32_t>(pathBytes.length().toRawValue()))
            .writeUInt16(static_cast<std::uint16_t>(hash.length().toRawValue()))
            .writeUInt16(0U)
            .writeBytes(identifierBytes)
            .writeBytes(pathBytes)
            .writeBytes(hash);
        return writer.toByteBlock().toUInt8Vector();
    }
    [[nodiscard]] static auto createMalformedInfo() -> std::vector<std::uint8_t> {
        auto result = createInfo("malformed.bin"_el, unit::ByteLength{}, unit::ByteLength{}, false, false);
        result[4U] = 2U;
        return result;
    }
};

const auto cPlainInfo =
    ResourceFixture::createInfo("plain.txt"_el, unit::ByteLength{5U}, unit::ByteLength{5U}, false, true);
const auto cEmptyInfo =
    ResourceFixture::createInfo("empty.txt"_el, unit::ByteLength{}, unit::ByteLength{}, false, false);
const auto cCompressedInfo =
    ResourceFixture::createInfo("compressed.txt"_el, unit::ByteLength{13U}, unit::ByteLength{13U}, true, true);
const auto cInvalidInfo =
    ResourceFixture::createInfo("invalid.bin"_el, unit::ByteLength{4U}, unit::ByteLength{2U}, true, false);
const auto cMalformedInfo = ResourceFixture::createMalformedInfo();

const auto cPlainRegistration = resource::impl::ResourceStorageRegistration{ResourceFixture::plainData, cPlainInfo};
const auto cEmptyRegistration = resource::impl::ResourceStorageRegistration{ResourceFixture::emptyData, cEmptyInfo};
const auto cCompressedRegistration =
    resource::impl::ResourceStorageRegistration{ResourceFixture::compressedData, cCompressedInfo};
const auto cInvalidRegistration =
    resource::impl::ResourceStorageRegistration{ResourceFixture::invalidData, cInvalidInfo};
const auto cMalformedRegistration =
    resource::impl::ResourceStorageRegistration{ResourceFixture::emptyData, cMalformedInfo};

}

using namespace el::text::literals;
using namespace erbsland::test::resourcemanagertest;

TESTED_TARGETS(ResourceInfo Resources ResourceManager ResourceError ResourceStorageRegistration ResourceStorageInfo)
class ResourceManagerTest final : public el::UnitTest {
public:
    void testLookupAndMissingDistinction() {
        const auto resources = el::resource::ResourceManager{};
        REQUIRE(resources.contains("test"_el, "plain.txt"_el));
        REQUIRE_FALSE(resources.contains("test"_el, "missing.txt"_el));
        REQUIRE_EQUAL(resources.getTextOrThrow("test"_el, "plain.txt"_el), "hello"_el);

        const auto empty = resources.getData("test"_el, "empty.txt"_el);
        REQUIRE(empty.has_value());
        REQUIRE(empty->isEmpty());
        REQUIRE_FALSE(resources.getData("test"_el, "missing.txt"_el).has_value());
        REQUIRE_THROWS_AS(el::resource::ResourceError, resources.getDataOrThrow("test"_el, "missing.txt"_el));
    }

    void testStoredAndLogicalData() {
        const auto resources = el::resource::ResourceManager{};
        const auto stored = resources.getStoredDataOrThrow("test"_el, "compressed.txt"_el);
        REQUIRE_EQUAL(stored.data(), el::mem::toConstByteSpan(ResourceFixture::compressedData()).data());
        REQUIRE_EQUAL(stored.size(), ResourceFixture::compressedData().size());
        REQUIRE_EQUAL(resources.getTextOrThrow("test"_el, "compressed.txt"_el), "abcdabcdabcde"_el);
    }

    void testMetadata() {
        const auto resources = el::resource::ResourceManager{};
        const auto compressed = resources.getInfoOrThrow("test"_el, "compressed.txt"_el);
        REQUIRE(compressed.isCompressed());
        REQUIRE(compressed.hasHash());
        REQUIRE_FALSE(compressed.isEncrypted());
        REQUIRE_EQUAL(compressed.originalSize(), el::unit::ByteLength{13U});
        REQUIRE_EQUAL(compressed.storedSize(), el::unit::ByteLength{13U});
        REQUIRE_EQUAL(*compressed.compressionAlgorithm(), el::mem::ByteCompressionAlgorithm::Lz4Block);
        REQUIRE_EQUAL(*compressed.hashAlgorithm(), el::cryptology::HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(compressed.hash().length(), el::unit::ByteLength{32U});
        REQUIRE_EQUAL(compressed.hash().getOrThrow(el::unit::ByteIndex{}), el::mem::Byte{0x42U});

        const auto plain = resources.getInfoOrThrow("test"_el, "plain.txt"_el);
        REQUIRE_FALSE(plain.isCompressed());
        REQUIRE(plain.hasHash());
    }

    void testInvalidData() {
        const auto resources = el::resource::ResourceManager{};
        REQUIRE_FALSE(resources.getData("test"_el, "invalid.bin"_el).has_value());
        try {
            static_cast<void>(resources.getDataOrThrow("test"_el, "invalid.bin"_el));
            REQUIRE(false);
        } catch (const el::resource::ResourceError &error) {
            REQUIRE_EQUAL(error.category(), el::resource::ResourceErrorCategory::InvalidData);
        }
    }

    void testMalformedMetadataIsIgnored() {
        const auto resources = el::resource::ResourceManager{};
        REQUIRE_FALSE(resources.contains("test"_el, "malformed.bin"_el));
    }

    void testTextCachingAndConcurrency() {
        const auto resources = el::resource::ResourceManager{};
        const auto first = resources.getTextOrThrow("test"_el, "plain.txt"_el);
        const auto second = resources.getTextOrThrow("test"_el, "plain.txt"_el);
        REQUIRE_EQUAL(first.storageId(), second.storageId());

        auto threads = std::vector<std::thread>{};
        auto valid = std::array<bool, 8U>{};
        for (auto index = std::size_t{}; index < valid.size(); ++index) {
            threads.emplace_back([&resources, &valid, index] {
                valid[index] = resources.getTextOrThrow("test"_el, "compressed.txt"_el) == "abcdabcdabcde"_el;
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        for (const auto result : valid) {
            REQUIRE(result);
        }
    }
};
