// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509ArtifactWriter.hpp"

#include "CryptologyOids.hpp"
#include "DerEncoder.hpp"
#include "DerParser.hpp"

#include "algorithm/rsa_signature/RsaSignature.hpp"

#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../CryptologyError.hpp"
#include "../HashAlgorithm.hpp"
#include "../Hasher.hpp"
#include "../keys/PublicKey.hpp"
#include "../tls/TlsSignatureScheme.hpp"
#include "../x509/X509KeyUsage.hpp"

#include "../../core/Application.hpp"
#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../network/IpVersion.hpp"
#include "../../random/Random.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../time/TimeAmounts.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../util/List.hpp"

#include <algorithm>

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto X509ArtifactWriter::createSelfSignedCertificate(const SigningPrivateKey &key) const -> X509Certificate {
    if (_builder._profile != X509CertificateProfile::CertificateAuthority) {
        throw err::LogicError{"Only a certificate-authority profile can create a self-signed certificate."_el};
    }
    validateIdentity();
    auto nameEncoder = DerEncoder{};
    appendSubjectName(nameEncoder);
    const auto name = nameEncoder.encoded();
    return createCertificateImpl(key, name, keyIdentifier(key.publicKey()), key, nullptr);
}

auto X509ArtifactWriter::createCertificate(
    const SigningPrivateKey &subjectKey,
    const X509Certificate &issuerCertificate,
    const SigningPrivateKey &issuerKey) const -> X509Certificate {
    validateIdentity();
    if (subjectKey.isEmpty() || issuerCertificate.isEmpty() || issuerKey.isEmpty()) {
        throw err::LogicError{"Certificate issuance requires subject key, issuer certificate, and issuer key."_el};
    }
    if (!issuerKey.matches(issuerCertificate.publicKey())) {
        throw err::ParameterError{"The issuer key does not match the issuer certificate."_el, "issuerKey"_el};
    }
    const auto constraints = issuerCertificate.basicConstraints();
    const auto usage = issuerCertificate.keyUsage();
    if (!constraints.has_value() || !constraints->isCertificateAuthority() || !usage.has_value() ||
        !usage->isSet(X509KeyUsage::KeyCertificateSign)) {
        throw err::ParameterError{
            "The issuer certificate is not authorized to sign certificates."_el, "issuerCertificate"_el};
    }
    if (_builder._profile == X509CertificateProfile::CertificateAuthority) {
        if (constraints->pathLength().has_value() && constraints->pathLength().value() == 0U) {
            throw err::ParameterError{
                "The issuer path-length constraint forbids a subordinate CA."_el, "issuerCertificate"_el};
        }
        if (constraints->pathLength().has_value() && _builder._caPathLength >= constraints->pathLength().value()) {
            throw err::ParameterError{
                "The subordinate CA path length is not below its issuer constraint."_el, "pathLength"_el};
        }
    }
    auto authority = issuerCertificate.subjectId();
    if (authority.isEmpty()) {
        authority = keyIdentifier(issuerCertificate.publicKey());
    }
    return createCertificateImpl(
        subjectKey, issuerCertificate.subject().asn1().encodedData(), authority, issuerKey, &issuerCertificate);
}

auto X509ArtifactWriter::createSigningRequest(const SigningPrivateKey &subjectKey) const
    -> X509CertificateSigningRequest {
    validateIdentity();
    if (subjectKey.isEmpty()) {
        throw err::LogicError{"A subject private key is required for a certificate signing request."_el};
    }
    auto infoEncoder = DerEncoder{};
    const auto info = infoEncoder.beginSequence();
    infoEncoder.appendPositiveInteger(uint64_t{});
    appendSubjectName(infoEncoder);
    infoEncoder.appendEncoded(subjectKey.publicKey().toDer().span());
    const auto attributes = infoEncoder.beginExplicit(0U);
    const auto attribute = infoEncoder.beginSequence();
    infoEncoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::extensionRequest));
    const auto attributeValues = infoEncoder.beginSet();
    appendExtensions(infoEncoder, subjectKey.publicKey(), {}, true);
    infoEncoder.end(attributeValues);
    infoEncoder.end(attribute);
    infoEncoder.end(attributes);
    infoEncoder.end(info);
    const auto requestInfo = infoEncoder.encoded();
    const auto scheme = signatureScheme(subjectKey);
    const auto algorithm = scheme.signatureAlgorithmIdentifier();
    // RFC 2986 section 4.2: sign the exact CertificationRequestInfo encoding with the subject key.
    const auto signature = subjectKey.signTlsCertificateVerify(scheme, requestInfo.span());
    if (!subjectKey.publicKey().verifySignature(algorithm, requestInfo.span(), signature.span())) {
        throw CryptologyError{"Generated certificate request failed its signature self-check."_el};
    }
    auto requestEncoder = DerEncoder{};
    const auto request = requestEncoder.beginSequence();
    requestEncoder.appendEncoded(requestInfo.span());
    requestEncoder.appendEncoded(algorithm.toDer().span());
    requestEncoder.appendBitString(signature.span());
    requestEncoder.end(request);
    auto result = requestEncoder.encoded();
    [[maybe_unused]] const auto validatedStructure = DerParser{result}.parseDocument();
    return X509CertificateSigningRequest{std::move(result)};
}

auto X509ArtifactWriter::createCertificateImpl(
    const SigningPrivateKey &subjectKey,
    const ByteBlock &issuerName,
    const ByteBlock &authorityKeyIdentifier,
    const SigningPrivateKey &issuerKey,
    const X509Certificate *const issuerCertificate) const -> X509Certificate {
    if (subjectKey.isEmpty() || issuerKey.isEmpty()) {
        throw err::LogicError{"Certificate creation requires nonempty signing keys."_el};
    }
    const auto now = time::DateTime::now();
    auto validFrom = _builder._validFrom.value_or(now - time::Duration{time::Minutes{5}});
    auto validTo = time::DateTime{};
    if (_builder._validTo.has_value()) {
        validTo = *_builder._validTo;
    } else if (_builder._lifetime.has_value()) {
        validTo = validFrom.addedOrThrow(*_builder._lifetime);
    } else if (_builder._profile == X509CertificateProfile::CertificateAuthority) {
        validTo = now.addedOrThrow(time::CalendarDelta{time::Years{10}});
    } else {
        validTo = now.addedOrThrow(time::CalendarDelta{time::Days{397}});
    }
    if (!validFrom.isValid() || !validTo.isValid() || validFrom >= validTo) {
        throw err::ParameterError{"Certificate validity must be a nonempty valid range."_el, "validity"_el};
    }
    if (issuerCertificate != nullptr) {
        if (_builder._validFrom.has_value() && validFrom < issuerCertificate->validFrom()) {
            throw err::ParameterError{"Explicit certificate validity starts before the issuer."_el, "validFrom"_el};
        }
        if ((_builder._validTo.has_value() || _builder._lifetime.has_value()) &&
            validTo > issuerCertificate->validTo()) {
            throw err::ParameterError{"Explicit certificate validity ends after the issuer."_el, "validTo"_el};
        }
        validFrom = std::max(validFrom, issuerCertificate->validFrom());
        validTo = std::min(validTo, issuerCertificate->validTo());
        if (validFrom >= validTo) {
            throw err::ParameterError{
                "Issuer validity does not overlap the requested certificate validity."_el, "validity"_el};
        }
    }
    const auto scheme = signatureScheme(issuerKey);
    const auto algorithm = scheme.signatureAlgorithmIdentifier();
    // RFC 5280 sections 4.1 and 4.1.2: build TBSCertificate in schema order and sign its exact DER encoding.
    auto tbsEncoder = DerEncoder{};
    const auto tbsScope = tbsEncoder.beginSequence();
    const auto version = tbsEncoder.beginExplicit(0U);
    tbsEncoder.appendPositiveInteger(uint64_t{2U});
    tbsEncoder.end(version);
    tbsEncoder.appendPositiveInteger(serialNumber().span());
    tbsEncoder.appendEncoded(algorithm.toDer().span());
    tbsEncoder.appendEncoded(issuerName.span());
    const auto validity = tbsEncoder.beginSequence();
    tbsEncoder.appendTime(validFrom);
    tbsEncoder.appendTime(validTo);
    tbsEncoder.end(validity);
    appendSubjectName(tbsEncoder);
    tbsEncoder.appendEncoded(subjectKey.publicKey().toDer().span());
    const auto extensionValues = tbsEncoder.beginExplicit(3U);
    appendExtensions(tbsEncoder, subjectKey.publicKey(), authorityKeyIdentifier, false);
    tbsEncoder.end(extensionValues);
    tbsEncoder.end(tbsScope);
    const auto tbs = tbsEncoder.encoded();
    const auto signature = issuerKey.signTlsCertificateVerify(scheme, tbs.span());
    auto certificateEncoder = DerEncoder{};
    const auto certificateScope = certificateEncoder.beginSequence();
    certificateEncoder.appendEncoded(tbs.span());
    certificateEncoder.appendEncoded(algorithm.toDer().span());
    certificateEncoder.appendBitString(signature.span());
    certificateEncoder.end(certificateScope);
    const auto certificate = X509Certificate::fromDerOrThrow(certificateEncoder.encoded());
    if (!issuerKey.publicKey().verifySignature(
            algorithm, certificate.tbsCertificateDer().span(), certificate.signatureData().span())) {
        throw CryptologyError{"Generated certificate failed its signature self-check."_el};
    }
    return certificate;
}

void X509ArtifactWriter::validateIdentity() const {
    if (!_builder._commonName.has_value() || _builder._commonName->isEmpty()) {
        throw err::ParameterError{"A nonempty common name is required."_el, "commonName"_el};
    }
    if ((_builder._profile == X509CertificateProfile::TlsServer ||
            _builder._profile == X509CertificateProfile::TlsServerAndClient) &&
        _builder._dnsNames.isEmpty() && _builder._ipAddresses.isEmpty()) {
        throw err::ParameterError{
            "A TLS server certificate requires at least one DNS name or IP address."_el, "subjectAlternativeNames"_el};
    }
    if (_builder._country.has_value() && _builder._country->characterLength().toSizeT() != 2U) {
        throw err::ParameterError{"The country code must contain exactly two characters."_el, "country"_el};
    }
}

void X509ArtifactWriter::appendSubjectName(DerEncoder &encoder) const {
    const auto name = encoder.beginSequence();
    const auto appendAttribute = [&](const String &id, const String &value, const bool printable) -> void {
        if (value.isEmpty()) {
            return;
        }
        const auto rdn = encoder.beginSet();
        const auto attribute = encoder.beginSequence();
        encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(id));
        if (printable) {
            encoder.appendPrintableString(value);
        } else {
            encoder.appendUtf8String(value);
        }
        encoder.end(attribute);
        encoder.end(rdn);
    };
    if (!_builder._sourceSubject.isEmpty()) {
        for (const auto &sourceRdn : _builder._sourceSubject.relativeDistinguishedNames()) {
            auto attributes = util::List<ByteBlock>{};
            for (const auto &attribute : sourceRdn.attributes()) {
                const auto id = attribute.oid().toString();
                const auto replaced = (id == cryptology_oids::commonName && _builder._commonName.has_value()) ||
                    (id == cryptology_oids::countryName && _builder._country.has_value()) ||
                    (id == cryptology_oids::stateOrProvinceName && _builder._state.has_value()) ||
                    (id == cryptology_oids::localityName && _builder._locality.has_value()) ||
                    (id == cryptology_oids::organizationName && _builder._organization.has_value()) ||
                    (id == cryptology_oids::organizationalUnitName && _builder._organizationalUnit.has_value());
                if (!replaced) {
                    auto attributeEncoder = DerEncoder{};
                    const auto attributeScope = attributeEncoder.beginSequence();
                    attributeEncoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(id));
                    attributeEncoder.appendEncoded(attribute.asn1().encodedData().span());
                    attributeEncoder.end(attributeScope);
                    attributes.append(attributeEncoder.encoded());
                }
            }
            if (!attributes.isEmpty()) {
                encoder.appendSet(std::move(attributes));
            }
        }
    }
    if (_builder._country.has_value()) {
        appendAttribute(cryptology_oids::countryName, *_builder._country, true);
    }
    if (_builder._state.has_value()) {
        appendAttribute(cryptology_oids::stateOrProvinceName, *_builder._state, false);
    }
    if (_builder._locality.has_value()) {
        appendAttribute(cryptology_oids::localityName, *_builder._locality, false);
    }
    if (_builder._organization.has_value()) {
        appendAttribute(cryptology_oids::organizationName, *_builder._organization, false);
    }
    if (_builder._organizationalUnit.has_value()) {
        appendAttribute(cryptology_oids::organizationalUnitName, *_builder._organizationalUnit, false);
    }
    appendAttribute(cryptology_oids::commonName, *_builder._commonName, false);
    encoder.end(name);
}

void X509ArtifactWriter::appendExtensions(
    DerEncoder &encoder,
    const PublicKey &subjectKey,
    const ByteBlock &authorityKeyIdentifier,
    const bool request) const {
    const auto root = encoder.beginSequence();
    const auto isCa = _builder._profile == X509CertificateProfile::CertificateAuthority;
    appendExtension(encoder, cryptology_oids::basicConstraints, isCa, [&](DerEncoder &valueEncoder) -> void {
        const auto constraints = valueEncoder.beginSequence();
        if (isCa) {
            valueEncoder.appendBoolean(true);
            valueEncoder.appendPositiveInteger(uint64_t{_builder._caPathLength});
        }
        valueEncoder.end(constraints);
    });

    auto usageByte = uint8_t{};
    auto unusedBits = uint8_t{};
    if (isCa) {
        usageByte = 0x06U;
        unusedBits = 1U;
    } else if (
        subjectKey.algorithm().oid().toString() == cryptology_oids::rsaEncryption ||
        subjectKey.algorithm().oid().toString() == cryptology_oids::rsaPss) {
        usageByte = 0xa0U;
        unusedBits = 5U;
    } else {
        usageByte = 0x80U;
        unusedBits = 7U;
    }
    const auto usageContent = ByteArray<1U>{Byte{usageByte}};
    appendExtension(encoder, cryptology_oids::keyUsage, true, [&](DerEncoder &valueEncoder) -> void {
        valueEncoder.appendBitString(usageContent.span(), unusedBits);
    });

    if (!isCa) {
        appendExtension(encoder, cryptology_oids::extendedKeyUsage, false, [&](DerEncoder &valueEncoder) -> void {
            const auto purposes = valueEncoder.beginSequence();
            if (_builder._profile == X509CertificateProfile::TlsServer ||
                _builder._profile == X509CertificateProfile::TlsServerAndClient) {
                valueEncoder.appendObjectIdentifier(
                    Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::serverAuth));
            }
            if (_builder._profile == X509CertificateProfile::TlsClient ||
                _builder._profile == X509CertificateProfile::TlsServerAndClient) {
                valueEncoder.appendObjectIdentifier(
                    Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::clientAuth));
            }
            valueEncoder.end(purposes);
        });
    }
    if (!_builder._dnsNames.isEmpty() || !_builder._ipAddresses.isEmpty()) {
        appendExtension(encoder, cryptology_oids::subjectAlternativeName, false, [&](DerEncoder &valueEncoder) -> void {
            static const auto cAsciiCharacters = CharSet::fromRange(Char{U'\0'}, Char{U'\x7f'});
            const auto names = valueEncoder.beginSequence();
            for (const auto &name : _builder._dnsNames) {
                if (!name.containsOnly(cAsciiCharacters)) {
                    throw err::ParameterError{"DNS subject alternative names must be ASCII."_el, "dnsName"_el};
                }
                const auto bytes = toConstByteSpan(text::impl::UnsafeU8StringAccess{name}.dataSpan());
                valueEncoder.appendImplicitPrimitive(2U, bytes);
            }
            for (const auto &address : _builder._ipAddresses) {
                const auto length = address.isV4() ? std::size_t{4U} : std::size_t{16U};
                valueEncoder.appendImplicitPrimitive(7U, address.bytes().span().first(length));
            }
            valueEncoder.end(names);
        });
    }
    for (const auto &preserved : _builder._preservedExtensions) {
        appendExtension(
            encoder, preserved.oid().toString(), preserved.isCritical(), [&](DerEncoder &valueEncoder) -> void {
                valueEncoder.appendEncoded(preserved.value().span());
            });
    }
    if (!request) {
        const auto subjectIdentifier = keyIdentifier(subjectKey);
        appendExtension(encoder, cryptology_oids::subjectKeyIdentifier, false, [&](DerEncoder &valueEncoder) -> void {
            valueEncoder.appendOctetString(subjectIdentifier.span());
        });
        appendExtension(encoder, cryptology_oids::authorityKeyIdentifier, false, [&](DerEncoder &valueEncoder) -> void {
            const auto identifier = valueEncoder.beginSequence();
            valueEncoder.appendImplicitPrimitive(0U, authorityKeyIdentifier.span());
            valueEncoder.end(identifier);
        });
    }
    encoder.end(root);
}

auto X509ArtifactWriter::signatureScheme(const SigningPrivateKey &key) const -> TlsSignatureScheme {
    switch (key.algorithm()) {
    case SigningKeyAlgorithm::Ed25519:
        return TlsSignatureScheme::Ed25519;
    case SigningKeyAlgorithm::EcdsaP256:
        return TlsSignatureScheme::EcdsaSecp256r1Sha256;
    case SigningKeyAlgorithm::EcdsaP384:
        return TlsSignatureScheme::EcdsaSecp384r1Sha384;
    case SigningKeyAlgorithm::Rsa: {
        const auto publicValues = rsa_signature::decodePublicKey(key.publicKey());
        const auto preferSha384 = publicValues.modulusBits >= 4096U;
        const auto rsae = key.publicKey().algorithm().oid().toString() == cryptology_oids::rsaEncryption;
        const auto preferred = TlsSignatureScheme{
            preferSha384 ? (rsae ? TlsSignatureScheme::RsaPssRsaeSha384 : TlsSignatureScheme::RsaPssPssSha384)
                         : (rsae ? TlsSignatureScheme::RsaPssRsaeSha256 : TlsSignatureScheme::RsaPssPssSha256)};
        if (key.supports(preferred)) {
            return preferred;
        }
        const auto fallback =
            TlsSignatureScheme{rsae ? TlsSignatureScheme::RsaPssRsaeSha256 : TlsSignatureScheme::RsaPssPssSha256};
        if (key.supports(fallback)) {
            return fallback;
        }
        break;
    }
    case SigningKeyAlgorithm::Unknown:
        break;
    }
    throw err::ParameterError{"The key has no supported X.509 signing scheme."_el, "key"_el};
}

auto X509ArtifactWriter::keyIdentifier(const PublicKey &key) -> ByteBlock {
    auto hasher = Hasher{HashAlgorithm::Sha1};
    hasher.update(key.keyData().span());
    return hasher.finalize();
}

auto X509ArtifactWriter::serialNumber() -> ByteBlock {
    auto random = core::application().secureRandom().buildByteBlock(ByteLength{20U});
    auto editor = ByteBlockEditor{random};
    editor.set(ByteIndex::zero(), Byte{static_cast<uint8_t>(editor.get(ByteIndex::zero()).toUInt8() & 0x7fU)});
    auto nonzero = uint8_t{};
    for (const auto byte : editor.span()) {
        nonzero |= byte.toUInt8();
    }
    if (nonzero == 0U) {
        editor.set(ByteIndex{19U}, Byte{1U});
    }
    return ByteBlock{editor};
}

}
