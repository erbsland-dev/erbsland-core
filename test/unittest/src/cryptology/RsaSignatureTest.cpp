// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/x509/X509AlgorithmIdentifier.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteIndex.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(RsaSignature PublicKey X509AlgorithmIdentifier)
class RsaSignatureTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testPssSha256KnownAnswer() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256.rsp"_el}.read();
        REQUIRE_EQUAL(records.size(), std::size_t{108U});
        const auto &record = records.front();
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));
        REQUIRE(publicKey.verifySignature(algorithm, message.span(), signature.span()));

        REQUIRE_FALSE(
            publicKey.verifySignature(algorithm, message.span(), signature.span().first(signature.span().size() - 1U)));
    }

    void testMalformedStateAndAlgorithm() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256.rsp"_el}.read();
        const auto &record = records.front();
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
        REQUIRE_THROWS_AS(
            el::err::LogicError, PublicKey{}.verifySignature(algorithm, message.span(), signature.span()));

        auto mixedHashAlgorithm = el::mem::ByteBlockEditor{algorithm.toDer()};
        mixedHashAlgorithm.set(
            el::unit::ByteIndex{59U}, mixedHashAlgorithm.get(el::unit::ByteIndex{59U}) ^ el::mem::Byte{3U});
        const auto malformed = X509AlgorithmIdentifier::fromDerOrThrow(
            mixedHashAlgorithm.slice(el::unit::ByteIndex::zero(), mixedHashAlgorithm.length()));
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        REQUIRE_THROWS_AS(el::err::ParseError, publicKey.verifySignature(malformed, message.span(), signature.span()));
    }
};
