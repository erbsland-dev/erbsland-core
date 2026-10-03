// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseKeys.hpp"
#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"
#include "../ParallelTestWork.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/HardwareFeatures.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableAesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableGaloisMultiplier.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringLiteral.hpp>

#if defined(ERBSLAND_AES_ARM_BACKEND)
#include <erbsland/cryptology/impl/algorithm/aes/ArmAesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/ArmGaloisMultiplier.hpp>
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
#include <erbsland/cryptology/impl/algorithm/aes/X86AesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/X86GaloisMultiplier.hpp>
#endif

#include <array>
#include <memory>
#include <utility>
#include <vector>

using el::cryptology::impl::AesBlockCipher;
using el::cryptology::impl::GaloisMultiplier;
using el::cryptology::impl::PortableAesBlockCipher;
using el::cryptology::impl::PortableGaloisMultiplier;
using el::mem::ByteArray;
using el::mem::ByteBlock;
using namespace el::text::literals;

TESTED_TARGETS(
    AesBlockCipher AesKeySchedule createAesBlockCipher PortableAesBlockCipher ArmAesBlockCipher X86AesBlockCipher
        GaloisMultiplier createGaloisMultiplier PortableGaloisMultiplier ArmGaloisMultiplier X86GaloisMultiplier
            hasArmAes hasArmPmull hasX86Aes hasX86Pclmul)
class AesPrimitiveFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    enum class Backend {
        Portable,
        Arm,
        X86,
    };

    struct TestFile final {
        el::StringLiteral name;
        std::size_t records;
        bool monteCarlo;
    };

    inline static constexpr auto files = std::array{
        TestFile{"ECBGFSbox128.rsp"_el, 14U, false},
        TestFile{"ECBGFSbox256.rsp"_el, 10U, false},
        TestFile{"ECBKeySbox128.rsp"_el, 42U, false},
        TestFile{"ECBKeySbox256.rsp"_el, 32U, false},
        TestFile{"ECBVarKey128.rsp"_el, 256U, false},
        TestFile{"ECBVarKey256.rsp"_el, 512U, false},
        TestFile{"ECBVarTxt128.rsp"_el, 256U, false},
        TestFile{"ECBVarTxt256.rsp"_el, 256U, false},
        TestFile{"ECBMMT128.rsp"_el, 20U, false},
        TestFile{"ECBMMT256.rsp"_el, 20U, false},
        TestFile{"ECBMCT128.rsp"_el, 200U, true},
        TestFile{"ECBMCT256.rsp"_el, 200U, true},
    };

    [[nodiscard]] static auto availableBackends() -> std::vector<Backend> {
        auto result = std::vector<Backend>{Backend::Portable};
#if defined(ERBSLAND_AES_ARM_BACKEND)
        if (el::cryptology::impl::hardware_features::hasArmAes()) {
            result.push_back(Backend::Arm);
        }
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
        if (el::cryptology::impl::hardware_features::hasX86Aes()) {
            result.push_back(Backend::X86);
        }
#endif
        return result;
    }

    [[nodiscard]] static auto availableMultiplierBackends() -> std::vector<Backend> {
        auto result = std::vector<Backend>{Backend::Portable};
#if defined(ERBSLAND_AES_ARM_BACKEND)
        if (el::cryptology::impl::hardware_features::hasArmPmull()) {
            result.push_back(Backend::Arm);
        }
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
        if (el::cryptology::impl::hardware_features::hasX86Pclmul()) {
            result.push_back(Backend::X86);
        }
#endif
        return result;
    }

    [[nodiscard]] static auto backendName(const Backend backend) -> el::StringLiteral {
        switch (backend) {
        case Backend::Portable:
            return "portable"_el;
        case Backend::Arm:
            return "ARM FEAT_AES"_el;
        case Backend::X86:
            return "x86 AES-NI"_el;
        }
        return "unknown"_el;
    }

    [[nodiscard]] static auto createCipher(const Backend backend, const el::mem::ConstByteSpan key)
        -> std::unique_ptr<AesBlockCipher> {
        switch (backend) {
        case Backend::Portable:
            return std::make_unique<PortableAesBlockCipher>(key);
#if defined(ERBSLAND_AES_ARM_BACKEND)
        case Backend::Arm:
            return std::make_unique<el::cryptology::impl::ArmAesBlockCipher>(key);
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
        case Backend::X86:
            return std::make_unique<el::cryptology::impl::X86AesBlockCipher>(key);
#endif
        default:
            throw el::RuntimeError{"AES backend is unavailable in this build."_el};
        }
    }

    [[nodiscard]] static auto createMultiplier(const Backend backend) -> std::unique_ptr<GaloisMultiplier> {
        switch (backend) {
        case Backend::Portable:
            return std::make_unique<PortableGaloisMultiplier>();
#if defined(ERBSLAND_AES_ARM_BACKEND)
        case Backend::Arm:
            return std::make_unique<el::cryptology::impl::ArmGaloisMultiplier>();
#endif
#if defined(ERBSLAND_AES_X86_BACKEND)
        case Backend::X86:
            return std::make_unique<el::cryptology::impl::X86GaloisMultiplier>();
#endif
        default:
            throw el::RuntimeError{"GHASH backend is unavailable in this build."_el};
        }
    }

    template <std::size_t N>
    [[nodiscard]] static auto arrayFrom(const ByteBlock &bytes) -> ByteArray<N> {
        return ByteArray<N>::fromSpanOrThrow(bytes.span());
    }

    [[nodiscard]] static auto calculateKnownAnswer(
        const CryptologyResponseReader::Record &record, const Backend backend, const bool chunkedBlocks)
        -> std::pair<ByteBlock, ByteBlock> {
        record.requireAllowedSettings({});
        record.requireAllowedValues(
            {"COUNT"_el,
                response_key::cEncryptionKeyField,
                response_key::cPlaintextField,
                response_key::cCiphertextField});
        record.requireValues(
            {"COUNT"_el,
                response_key::cEncryptionKeyField,
                response_key::cPlaintextField,
                response_key::cCiphertextField});
        const auto key = bytesFromHex(record.value(response_key::cEncryptionKeyField));
        const auto plaintext = bytesFromHex(record.value(response_key::cPlaintextField));
        const auto ciphertext = bytesFromHex(record.value(response_key::cCiphertextField));
        if (plaintext.length() != ciphertext.length() || plaintext.length().toSizeT() % 16U != 0U) {
            throw el::RuntimeError{"Invalid AES response block lengths."_el};
        }
        auto cipher = createCipher(backend, key.span());
        auto actual = el::mem::ByteBlockEditor{};
        for (auto offset = std::size_t{}; offset < plaintext.length().toSizeT(); offset += 16U) {
            const auto input = record.direction == CryptologyResponseReader::Direction::Encrypt
                ? arrayFrom<16>(ByteBlock::fromSpan(plaintext.span().subspan(offset, 16U)))
                : arrayFrom<16>(ByteBlock::fromSpan(ciphertext.span().subspan(offset, 16U)));
            const auto output = record.direction == CryptologyResponseReader::Direction::Encrypt
                ? cipher->encrypt(input)
                : cipher->decrypt(input);
            actual.append(output.span());
            if (chunkedBlocks) {
                cipher->secureErase();
                cipher = createCipher(backend, key.span());
            }
        }
        return {
            ByteBlock{actual},
            record.direction == CryptologyResponseReader::Direction::Encrypt ? ciphertext : plaintext};
    }

    [[nodiscard]] static auto calculateMonteCarlo(const CryptologyResponseReader::Record &record, const Backend backend)
        -> std::pair<ByteBlock, ByteBlock> {
        record.requireAllowedSettings({});
        record.requireAllowedValues(
            {"COUNT"_el,
                response_key::cEncryptionKeyField,
                response_key::cPlaintextField,
                response_key::cCiphertextField});
        record.requireValues(
            {"COUNT"_el,
                response_key::cEncryptionKeyField,
                response_key::cPlaintextField,
                response_key::cCiphertextField});
        const auto key = bytesFromHex(record.value(response_key::cEncryptionKeyField));
        auto value = record.direction == CryptologyResponseReader::Direction::Encrypt
            ? arrayFrom<16>(bytesFromHex(record.value(response_key::cPlaintextField)))
            : arrayFrom<16>(bytesFromHex(record.value(response_key::cCiphertextField)));
        auto cipher = createCipher(backend, key.span());
        for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
            value = record.direction == CryptologyResponseReader::Direction::Encrypt ? cipher->encrypt(value)
                                                                                     : cipher->decrypt(value);
        }
        const auto expected = record.direction == CryptologyResponseReader::Direction::Encrypt
            ? arrayFrom<16>(bytesFromHex(record.value(response_key::cCiphertextField)))
            : arrayFrom<16>(bytesFromHex(record.value(response_key::cPlaintextField)));
        return {ByteBlock::fromSpan(value.span()), ByteBlock::fromSpan(expected.span())};
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAutomaticBackendSelection() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto key = bytesFromHex("000102030405060708090a0b0c0d0e0f"_el);
        const auto cipher = el::cryptology::impl::createAesBlockCipher(key.span());
        const auto multiplier = el::cryptology::impl::createGaloisMultiplier();

#if defined(ERBSLAND_AES_ARM_BACKEND)
        if (el::cryptology::impl::hardware_features::hasArmAes()) {
            REQUIRE(dynamic_cast<el::cryptology::impl::ArmAesBlockCipher *>(cipher.get()) != nullptr);
        } else {
            REQUIRE(dynamic_cast<PortableAesBlockCipher *>(cipher.get()) != nullptr);
        }
        if (el::cryptology::impl::hardware_features::hasArmPmull()) {
            REQUIRE(dynamic_cast<el::cryptology::impl::ArmGaloisMultiplier *>(multiplier.get()) != nullptr);
        } else {
            REQUIRE(dynamic_cast<el::cryptology::impl::PortableGaloisMultiplier *>(multiplier.get()) != nullptr);
        }
#elif defined(ERBSLAND_AES_X86_BACKEND)
        if (el::cryptology::impl::hardware_features::hasX86Aes()) {
            REQUIRE(dynamic_cast<el::cryptology::impl::X86AesBlockCipher *>(cipher.get()) != nullptr);
        } else {
            REQUIRE(dynamic_cast<PortableAesBlockCipher *>(cipher.get()) != nullptr);
        }
        if (el::cryptology::impl::hardware_features::hasX86Pclmul()) {
            REQUIRE(dynamic_cast<el::cryptology::impl::X86GaloisMultiplier *>(multiplier.get()) != nullptr);
        } else {
            REQUIRE(dynamic_cast<el::cryptology::impl::PortableGaloisMultiplier *>(multiplier.get()) != nullptr);
        }
#else
        REQUIRE(dynamic_cast<PortableAesBlockCipher *>(cipher.get()) != nullptr);
        REQUIRE(dynamic_cast<el::cryptology::impl::PortableGaloisMultiplier *>(multiplier.get()) != nullptr);
#endif
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testGaloisBackendDifferential() {
        const auto portable = PortableGaloisMultiplier{};
        for (const auto backend : availableMultiplierBackends()) {
            const auto multiplier = createMultiplier(backend);
            for (auto iteration = std::size_t{}; iteration < 4096U; ++iteration) {
                auto left = GaloisMultiplier::Block{};
                auto right = GaloisMultiplier::Block{};
                for (
                    auto index = el::unit::ByteIndex::zero(); index.isWithin(el::unit::ByteCount{16U});
                    index.uncheckedIncrement()) {
                    left.set(
                        index,
                        el::mem::Byte{static_cast<uint8_t>((iteration * 17U + index.toRawValue() * 29U) & 0xffU)});
                    right.set(
                        index,
                        el::mem::Byte{static_cast<uint8_t>((iteration * 43U + index.toRawValue() * 11U + 7U) & 0xffU)});
                }
                REQUIRE_EQUAL(multiplier->multiply(left, right), portable.multiply(left, right));
            }
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfficialEcbVectors() {
        for (const auto backend : availableBackends()) {
            for (const auto &file : files) {
                const auto records = CryptologyResponseReader{el::Path{"data/cryptology/aes"_el} / file.name}.read();
                REQUIRE_EQUAL(records.size(), file.records);
                const auto results = parallel_test_work::run(
                    records.size(), [&](const std::size_t index) -> std::pair<ByteBlock, ByteBlock> {
                        if (file.monteCarlo) {
                            return calculateMonteCarlo(records[index], backend);
                        }
                        return calculateKnownAnswer(
                            records[index], backend, el::String{file.name}.startsWith("ECBMMT"_el));
                    });
                auto directionCounts = std::array<std::size_t, 2>{};
                for (auto index = std::size_t{}; index < records.size(); ++index) {
                    const auto &record = records[index];
                    REQUIRE(
                        record.direction == CryptologyResponseReader::Direction::Encrypt ||
                        record.direction == CryptologyResponseReader::Direction::Decrypt);
                    const auto directionIndex =
                        record.direction == CryptologyResponseReader::Direction::Encrypt ? 0U : 1U;
                    REQUIRE_EQUAL(record.unsignedValue("COUNT"_el), directionCounts[directionIndex]);
                    ++directionCounts[directionIndex];
                    runWithContext(
                        SOURCE_LOCATION(),
                        [&]() -> void {
                            REQUIRE_NOTHROW(results[index].value());
                            const auto &[actual, expected] = results[index].value();
                            REQUIRE_EQUAL(actual, expected);
                        },
                        [&]() -> std::string {
                            return el::StringConverter{
                                el::StringFormat{"{} backend {}"_el}.build(record.diagnostic(), backendName(backend))}
                                .toStdString();
                        });
                }
                REQUIRE_EQUAL(directionCounts, (std::array<std::size_t, 2>{file.records / 2U, file.records / 2U}));
            }
        }
    }
};
