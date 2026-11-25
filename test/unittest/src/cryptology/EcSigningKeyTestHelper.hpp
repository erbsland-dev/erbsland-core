// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptologyTestHelper.hpp"

#include <erbsland/asn1/Asn1ObjectIdentifier.hpp>
#include <erbsland/Byte.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/cryptology/impl/algorithm/ecdsa_signature/EcdsaSigner.hpp>
#include <erbsland/cryptology/impl/algorithm/NistPrimeCurve.hpp>
#include <erbsland/cryptology/impl/CryptologyOids.hpp>
#include <erbsland/der/DerEncoder.hpp>
#include <erbsland/der/DerParser.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/SigningKeyProfile.hpp>
#include <erbsland/SigningPrivateKey.hpp>

#include <array>
#include <cstdint>

/// Public scalar fixtures and PKCS#8 construction shared by EC policy tests.
class EcSigningKeyTestHelper : public CryptologyTestHelper {
protected:
    /// Supported curve selector.
    using CurveName = el::cryptology::impl::NistPrimeCurve::Name;
    /// Curves covered by every integration scenario.
    static constexpr auto cCurves = std::array{CurveName::P256, CurveName::P384};

protected: // fixtures
    /// Construct a zero-padded scalar containing one low byte for the selected curve.
    /// @param name Selected curve.
    /// @param lastByte Low byte of the scalar.
    /// @return Exact-width big-endian scalar.
    [[nodiscard]] static auto scalarBytes(const CurveName name, const uint8_t lastByte = 4U) -> el::ByteBlock {
        const auto curve = el::cryptology::impl::NistPrimeCurve{name};
        auto bytes = el::ByteBlockEditor{curve.byteLength()};
        bytes.fill(el::Byte{});
        bytes.set(bytes.lastIndex(), el::Byte{lastByte});
        return bytes;
    }
    /// Construct a deterministic interior scalar for either curve.
    /// @param name Selected curve.
    /// @return Exact-width scalar filled with 0x42.
    [[nodiscard]] static auto acceptedScalar(const CurveName name) -> el::ByteBlock {
        auto bytes = el::ByteBlockEditor{scalarBytes(name)};
        bytes.fill(el::Byte{0x42U});
        return bytes;
    }
    /// Select the signing-key generation profile for a curve.
    /// @param name Selected curve.
    /// @return Corresponding ECDSA profile.
    [[nodiscard]] static auto profile(const CurveName name) -> el::cryptology::SigningKeyProfile {
        if (name == CurveName::P256) {
            return el::cryptology::SigningKeyProfile::EcdsaP256;
        }
        return el::cryptology::SigningKeyProfile::EcdsaP384;
    }
    /// Encode an EC PKCS#8 fixture, optionally deriving the embedded public point through the primitive.
    /// @param name Selected curve.
    /// @param scalar Scalar bytes, including intentionally invalid encodings.
    /// @param includePublicKey Derive and embed a public point; requires a mathematically valid scalar.
    /// @return Encoded PKCS#8 test key.
    [[nodiscard]] static auto privateKeyDer(
        const CurveName name, const el::ByteBlock &scalar, const bool includePublicKey = false) -> el::ByteBlock {
        using namespace el::cryptology;
        using namespace el::cryptology::impl;
        auto encoder = el::data::der::DerEncoder{};
        encoder.markAsSensitive();
        const auto root = encoder.beginSequence();
        encoder.appendPositiveInteger(uint64_t{});
        const auto algorithm = encoder.beginSequence();
        encoder.appendObjectIdentifier(
            el::data::asn1::Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::ecPublicKey));
        auto curveOid = cryptology_oids::secp256r1;
        if (name == CurveName::P384) {
            curveOid = cryptology_oids::secp384r1;
        }
        encoder.appendObjectIdentifier(el::data::asn1::Asn1ObjectIdentifier::fromStringOrThrow(curveOid));
        encoder.end(algorithm);
        const auto wrapped = encoder.beginOctetString();
        const auto ecKey = encoder.beginSequence();
        encoder.appendPositiveInteger(uint64_t{1U});
        encoder.appendOctetString(scalar.span());
        if (includePublicKey) {
            const auto publicKey = encoder.beginExplicit(1U);
            encoder.appendBitString(ecdsa_signer::publicKey(scalar.span(), name).span());
            encoder.end(publicKey);
        }
        encoder.end(ecKey);
        encoder.end(wrapped);
        encoder.end(root);
        return encoder.encoded();
    }
    /// Extract the actual scalar from an accepted key to check candidate selection.
    /// @param key Accepted ECDSA key.
    /// @return Serialized scalar bytes.
    [[nodiscard]] static auto exportedScalar(const el::cryptology::SigningPrivateKey &key) -> el::ByteBlock {
        const auto root = el::data::der::DerParser::parseOrThrow(key.toDer());
        const auto ecKey = el::data::der::DerParser::parseOrThrow(root.child(el::ItemIndex{2U}).contentData());
        return ecKey.child(el::ItemIndex{1U}).contentData();
    }
};
