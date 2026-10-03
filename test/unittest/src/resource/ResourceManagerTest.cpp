// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockLiteral.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteSpan.hpp>
#include <erbsland/CompressionAlgorithm.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ResourceError.hpp>
#include <erbsland/ResourceErrorCategory.hpp>
#include <erbsland/ResourceInfo.hpp>
#include <erbsland/ResourceManager.hpp>
#include <erbsland/ResourceStorageRegistration.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <future>

using namespace el;

TESTED_TARGETS(ResourceInfo Resources ResourceManager ResourceError ResourceStorageRegistration ResourceStorageInfo)
class ResourceManagerTest final : public UnitTest {
private:
    static constexpr auto cIdentifier = "test"_el;
    static constexpr auto cPlainPath = "plain.txt"_el;
    static constexpr auto cEmptyPath = "empty.txt"_el;
    static constexpr auto cCompressedPath = "compressed.txt"_el;
    static constexpr auto cPlainText = "hello"_el;
    static constexpr auto cCompressedText = "Compiled resources preserve the original bytes.\n"
                                            "Compiled resources preserve the original bytes.\n"
                                            "Compiled resources preserve the original bytes.\n"
                                            "Compiled resources preserve the original bytes.\n"_el;

public: // tests
    void testLookupAndMissingDistinction() {
        const auto resources = ResourceManager{};
        REQUIRE(resources.contains(cIdentifier, cPlainPath));
        REQUIRE_EQUAL(resources.getTextOrThrow(cIdentifier, cPlainPath), cPlainText);
        WITH_CONTEXT(requireMissing(resources, cIdentifier, "missing.txt"_el));
        WITH_CONTEXT(requireMissing(resources, "missing"_el, cPlainPath));
        WITH_CONTEXT(requireMissing(resources, "Test"_el, cPlainPath));
        WITH_CONTEXT(requireMissing(resources, cIdentifier, "Plain.txt"_el));
    }

    void testEmptyResource() {
        const auto resources = ResourceManager{};
        REQUIRE(resources.contains(cIdentifier, cEmptyPath));
        const auto stored = resources.getStoredData(cIdentifier, cEmptyPath);
        REQUIRE(stored.has_value());
        REQUIRE(stored->empty());
        const auto data = resources.getData(cIdentifier, cEmptyPath);
        REQUIRE(data.has_value());
        REQUIRE(data->isEmpty());
        const auto text = resources.getText(cIdentifier, cEmptyPath);
        REQUIRE(text.has_value());
        REQUIRE(text->isEmpty());
        REQUIRE(resources.getStoredDataOrThrow(cIdentifier, cEmptyPath).empty());
        REQUIRE(resources.getDataOrThrow(cIdentifier, cEmptyPath).isEmpty());
        REQUIRE(resources.getTextOrThrow(cIdentifier, cEmptyPath).isEmpty());
    }

    void testStoredAndLogicalData() {
        const auto resources = ResourceManager{};
        const auto plainStored = resources.getStoredDataOrThrow(cIdentifier, cPlainPath);
        const auto plain = resources.getDataOrThrow(cIdentifier, cPlainPath);
        REQUIRE_EQUAL(plain, utf8(cPlainText));
        REQUIRE_EQUAL(plain.span().data(), plainStored.data());
        REQUIRE_EQUAL(plain.span().size(), plainStored.size());

        const auto compressedStored = resources.getStoredDataOrThrow(cIdentifier, cCompressedPath);
        const auto compressed = resources.getDataOrThrow(cIdentifier, cCompressedPath);
        REQUIRE_EQUAL(compressed, utf8(cCompressedText));
        REQUIRE_LESS(compressedStored.size(), compressed.span().size());
        REQUIRE_NOT_EQUAL(compressed.span().data(), compressedStored.data());
        REQUIRE_EQUAL(resources.getTextOrThrow(cIdentifier, cCompressedPath), cCompressedText);
        const auto storedAgain = resources.getStoredDataOrThrow(cIdentifier, cCompressedPath);
        REQUIRE_EQUAL(storedAgain.data(), compressedStored.data());
        REQUIRE_EQUAL(storedAgain.size(), compressedStored.size());
    }

    void testMetadata() {
        const auto resources = ResourceManager{};
        const auto compressed = resources.getInfoOrThrow(cIdentifier, cCompressedPath);
        REQUIRE(compressed.isCompressed());
        REQUIRE(compressed.compressionAlgorithm().has_value());
        REQUIRE_EQUAL(*compressed.compressionAlgorithm(), CompressionAlgorithm::Lz4Block);
        REQUIRE_EQUAL(compressed.originalSize(), utf8(cCompressedText).length());
        REQUIRE_EQUAL(
            compressed.storedSize(),
            ByteCount::fromSizeT(resources.getStoredDataOrThrow(cIdentifier, cCompressedPath).size()));
        REQUIRE_LESS(compressed.storedSize(), compressed.originalSize());
        WITH_CONTEXT(requireHash(compressed, cCompressedText));
        REQUIRE_FALSE(compressed.isEncrypted());

        const auto plain = resources.getInfoOrThrow(cIdentifier, cPlainPath);
        REQUIRE_FALSE(plain.isCompressed());
        REQUIRE_FALSE(plain.compressionAlgorithm().has_value());
        REQUIRE_EQUAL(plain.originalSize(), utf8(cPlainText).length());
        REQUIRE_EQUAL(plain.storedSize(), plain.originalSize());
        WITH_CONTEXT(requireHash(plain, cPlainText));
        REQUIRE_FALSE(plain.isEncrypted());

        const auto empty = resources.getInfoOrThrow(cIdentifier, cEmptyPath);
        REQUIRE_EQUAL(empty.originalSize(), ByteCount{});
        REQUIRE_EQUAL(empty.storedSize(), ByteCount{});
        REQUIRE_FALSE(empty.isCompressed());
        REQUIRE_FALSE(empty.compressionAlgorithm().has_value());
        REQUIRE_FALSE(empty.hasHash());
        REQUIRE_FALSE(empty.hashAlgorithm().has_value());
        REQUIRE(empty.hash().isEmpty());
        REQUIRE_FALSE(empty.isEncrypted());
    }

    void testInvalidData() {
        // Valid version-1 metadata for invalid/data: LZ4, four logical bytes, two stored bytes, no hash.
        // This deliberately corrupt stream cannot be produced by the resource compiler.
        // clang-format off
        static constexpr std::uint8_t cInfo[] = {
            'E', 'L', 'R', 'I', 1, 1, 0, 0,
            4, 0, 0, 0, 0, 0, 0, 0,
            2, 0, 0, 0, 0, 0, 0, 0,
            7, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0,
            'i', 'n', 'v', 'a', 'l', 'i', 'd', 'd', 'a', 't', 'a',
        };
        // clang-format on
        // The intrusive registry and both byte blocks must remain alive for the rest of the process.
        static const auto cRegistration =
            resource::impl::ResourceStorageRegistration{invalidData, ByteBlockLiteral{cInfo}};
        const auto resources = ResourceManager{};
        const auto identifier = "invalid"_el;
        const auto path = "data"_el;
        REQUIRE(resources.contains(identifier, path));
        REQUIRE(resources.getInfoOrThrow(identifier, path).isCompressed());
        REQUIRE_EQUAL(ByteBlock::fromSpan(resources.getStoredDataOrThrow(identifier, path)), ByteBlock{invalidData()});
        REQUIRE_FALSE(resources.getData(identifier, path).has_value());
        REQUIRE_FALSE(resources.getText(identifier, path).has_value());
        WITH_CONTEXT(requireError(ResourceErrorCategory::InvalidData, [&]() -> ByteBlock {
            return resources.getDataOrThrow(identifier, path);
        }));
        WITH_CONTEXT(requireError(ResourceErrorCategory::InvalidData, [&]() -> String {
            return resources.getTextOrThrow(identifier, path);
        }));
    }

    void testMalformedMetadataIsIgnored() {
        // Otherwise valid metadata for malformed/data, with the unsupported format version 2.
        // clang-format off
        static constexpr std::uint8_t cInfo[] = {
            'E', 'L', 'R', 'I', 2, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            9, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0,
            'm', 'a', 'l', 'f', 'o', 'r', 'm', 'e', 'd', 'd', 'a', 't', 'a',
        };
        // clang-format on
        static const auto cRegistration =
            resource::impl::ResourceStorageRegistration{emptyData, ByteBlockLiteral{cInfo}};
        const auto resources = ResourceManager{};
        WITH_CONTEXT(requireMissing(resources, "malformed"_el, "data"_el));
        REQUIRE_EQUAL(resources.getTextOrThrow(cIdentifier, cPlainPath), cPlainText);
    }

    void testDecodedValuesAreCached() {
        const auto resources = ResourceManager{};
        const auto firstData = resources.getDataOrThrow(cIdentifier, cCompressedPath);
        const auto secondData = resources.getDataOrThrow(cIdentifier, cCompressedPath);
        REQUIRE_EQUAL(firstData, utf8(cCompressedText));
        REQUIRE_EQUAL(firstData.span().data(), secondData.span().data());
        const auto firstText = resources.getTextOrThrow(cIdentifier, cCompressedPath);
        const auto secondText = resources.getTextOrThrow(cIdentifier, cCompressedPath);
        REQUIRE_EQUAL(firstText, cCompressedText);
        REQUIRE_EQUAL(firstText.storageId(), secondText.storageId());
    }

    void testConcurrentFirstAccess() {
        const auto resources = ResourceManager{};
        auto start = std::barrier{8};
        auto readers = std::array<std::future<String>, 8>{};
        for (auto &reader : readers) {
            reader = std::async(std::launch::async, [&]() -> String {
                start.arrive_and_wait();
                return resources.getTextOrThrow(cIdentifier, cCompressedPath);
            });
        }
        // Futures propagate worker exceptions to the test runner and join on destruction.
        const auto first = readers.front().get();
        REQUIRE_EQUAL(first, cCompressedText);
        for (auto index = std::size_t{1}; index < readers.size(); ++index) {
            const auto result = readers[index].get();
            REQUIRE_EQUAL(result, cCompressedText);
            REQUIRE_EQUAL(result.storageId(), first.storageId());
        }
    }

private:
    [[nodiscard]] static auto utf8(const String &text) -> ByteBlock {
        return StringEncoder{text}.encode(StringEncoding::Utf8, StringBomMode::Reject);
    }

    [[nodiscard]] static auto invalidData() noexcept -> ByteBlockLiteral {
        return ByteBlockLiteral::fromValues<0, 0>();
    }

    [[nodiscard]] static auto emptyData() noexcept -> ByteBlockLiteral { return {}; }

    void requireHash(const ResourceInfo &info, const String &text) {
        REQUIRE(info.hasHash());
        REQUIRE(info.hashAlgorithm().has_value());
        REQUIRE_EQUAL(*info.hashAlgorithm(), HashAlgorithm::Sha3_256);
        auto hasher = Hasher{HashAlgorithm::Sha3_256};
        hasher.update(utf8(text));
        REQUIRE_EQUAL(info.hash(), hasher.finalize());
    }

    void requireMissing(const ResourceManager &resources, const String &identifier, const String &path) {
        REQUIRE_FALSE(resources.contains(identifier, path));
        REQUIRE_FALSE(resources.getStoredData(identifier, path).has_value());
        REQUIRE_FALSE(resources.getData(identifier, path).has_value());
        REQUIRE_FALSE(resources.getText(identifier, path).has_value());
        REQUIRE_FALSE(resources.getInfo(identifier, path).has_value());
        WITH_CONTEXT(requireError(ResourceErrorCategory::NotFound, [&]() -> ConstByteSpan {
            return resources.getStoredDataOrThrow(identifier, path);
        }));
        WITH_CONTEXT(requireError(ResourceErrorCategory::NotFound, [&]() -> ByteBlock {
            return resources.getDataOrThrow(identifier, path);
        }));
        WITH_CONTEXT(requireError(
            ResourceErrorCategory::NotFound, [&]() -> String { return resources.getTextOrThrow(identifier, path); }));
        WITH_CONTEXT(requireError(ResourceErrorCategory::NotFound, [&]() -> ResourceInfo {
            return resources.getInfoOrThrow(identifier, path);
        }));
    }

    template <typename T>
    void requireError(const ResourceErrorCategory expectedCategory, const T &action) {
        try {
            [[maybe_unused]] const auto result = action();
        } catch (const ResourceError &error) {
            REQUIRE_EQUAL(error.category(), expectedCategory);
            return;
        }
        REQUIRE(false);
    }
};
