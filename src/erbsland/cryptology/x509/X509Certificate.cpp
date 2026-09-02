// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509Certificate.hpp"

#include "../impl/PemCodec.hpp"
#include "../impl/PemDerFileTools.hpp"
#include "../impl/PortableX509CertificateData.hpp"
#include "../impl/X509Parser.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParseError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../path/Path.hpp"
#include "../../text/base_n/BaseNEncoder.hpp"
#include "../../text/base_n/BaseNFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

auto X509Certificate::portableData() const -> const impl::PortableX509CertificateData * {
    return isEmpty() ? nullptr : &_data.constGet()->portableData();
}

auto X509Certificate::version() const noexcept -> X509Version {
    const auto *data = portableData();
    return data == nullptr ? X509Version::Unknown : data->values().version;
}

auto X509Certificate::serialNumber() const -> mem::ByteBlock {
    const auto *data = portableData();
    return data == nullptr ? mem::ByteBlock{} : data->values().serialNumber;
}

auto X509Certificate::signatureAlgorithm() const -> X509AlgorithmIdentifier {
    const auto *data = portableData();
    return data == nullptr ? X509AlgorithmIdentifier{} : data->values().signatureAlgorithm;
}

auto X509Certificate::signatureAlgorithmId() const -> text::String {
    return signatureAlgorithm().oid().toString();
}

auto X509Certificate::issuer() const -> X509Name {
    const auto *data = portableData();
    return data == nullptr ? X509Name{} : data->values().issuer;
}

auto X509Certificate::issuerId() const -> mem::ByteBlock {
    const auto *data = portableData();
    return data == nullptr ? mem::ByteBlock{} : data->values().authorityKeyIdentifier;
}

auto X509Certificate::validFrom() const noexcept -> time::DateTime {
    const auto *data = portableData();
    return data == nullptr ? time::DateTime{} : data->values().validFrom;
}

auto X509Certificate::validTo() const noexcept -> time::DateTime {
    const auto *data = portableData();
    return data == nullptr ? time::DateTime{} : data->values().validTo;
}

auto X509Certificate::subject() const -> X509Name {
    const auto *data = portableData();
    return data == nullptr ? X509Name{} : data->values().subject;
}

auto X509Certificate::subjectId() const -> mem::ByteBlock {
    const auto *data = portableData();
    return data == nullptr ? mem::ByteBlock{} : data->values().subjectKeyIdentifier;
}

auto X509Certificate::subjectAlternativeNames() const -> util::List<X509GeneralName> {
    const auto *data = portableData();
    return data == nullptr ? util::List<X509GeneralName>{} : data->values().subjectAlternativeNames;
}

auto X509Certificate::dnsNames() const -> text::StringList {
    auto result = text::StringList{};
    for (const auto &name : subjectAlternativeNames()) {
        if (name.kind() == X509GeneralName::Kind::Dns) {
            result.append(name.text());
        }
    }
    return result;
}

auto X509Certificate::ipAddresses() const -> util::List<network::IpAddress> {
    auto result = util::List<network::IpAddress>{};
    for (const auto &name : subjectAlternativeNames()) {
        if (name.kind() == X509GeneralName::Kind::IpAddress && name.ipAddress().has_value()) {
            result.append(*name.ipAddress());
        }
    }
    return result;
}

auto X509Certificate::publicKey() const -> PublicKey {
    const auto *data = portableData();
    return data == nullptr ? PublicKey{} : data->values().publicKey;
}

auto X509Certificate::tbsCertificateDer() const -> mem::ByteBlock {
    const auto *data = portableData();
    return data == nullptr ? mem::ByteBlock{} : data->values().tbsNode.encodedData();
}

auto X509Certificate::tbsSignatureAlgorithm() const -> X509AlgorithmIdentifier {
    const auto *data = portableData();
    return data == nullptr ? X509AlgorithmIdentifier{} : data->values().tbsSignatureAlgorithm;
}

auto X509Certificate::signatureData() const -> mem::ByteBlock {
    const auto *data = portableData();
    return data == nullptr ? mem::ByteBlock{} : data->values().signature;
}

auto X509Certificate::signatureUnusedBitCount() const noexcept -> uint8_t {
    const auto *data = portableData();
    return data == nullptr ? 0U : data->values().signatureUnusedBitCount;
}

auto X509Certificate::extensions() const -> util::List<X509Extension> {
    const auto *data = portableData();
    return data == nullptr ? util::List<X509Extension>{} : data->values().extensions;
}

auto X509Certificate::basicConstraints() const -> std::optional<X509BasicConstraints> {
    const auto *data = portableData();
    return data == nullptr ? std::nullopt : data->values().basicConstraints;
}

auto X509Certificate::keyUsage() const -> std::optional<X509KeyUsages> {
    const auto *data = portableData();
    return data == nullptr ? std::nullopt : data->values().keyUsage;
}

auto X509Certificate::extendedKeyUsage() const -> util::List<Asn1ObjectIdentifier> {
    const auto *data = portableData();
    return data == nullptr ? util::List<Asn1ObjectIdentifier>{} : data->values().extendedKeyUsage;
}

auto X509Certificate::profileIssues() const -> util::List<X509CertificateProfileIssue> {
    const auto *data = portableData();
    return data == nullptr ? util::List<X509CertificateProfileIssue>{} : data->values().profileIssues;
}

auto X509Certificate::asn1() const -> Asn1Node {
    const auto *data = portableData();
    return data == nullptr ? Asn1Node{} : data->values().root;
}

auto X509Certificate::toPem() const -> text::String {
    return isEmpty() ? text::String{} : impl::PemCodec{toDer()}.encode();
}

auto X509Certificate::toDer() const -> mem::ByteBlock {
    const auto *data = portableData();
    return data == nullptr ? mem::ByteBlock{} : data->values().root.encodedData();
}

auto X509Certificate::toString() const -> text::String {
    if (isEmpty()) {
        return {};
    }
    const auto serial = text::base_n::BaseNEncoder{serialNumber(), text::base_n::BaseNFormat::base16()}.toString();
    auto result = text::StringEditor{};
    result.append("Subject: "_el);
    result.append(subject().toString());
    result.append(", Issuer: "_el);
    result.append(issuer().toString());
    result.append(", Serial: "_el);
    result.append(serial);
    return text::String{result};
}

auto X509Certificate::toStringTree() const -> text::StringTree {
    if (isEmpty()) {
        return {};
    }
    auto result = text::StringTree{"X.509 Certificate"_el};
    auto versionText = "unknown"_el;
    if (version() == X509Version::V1) {
        versionText = "v1"_el;
    } else if (version() == X509Version::V2) {
        versionText = "v2"_el;
    } else if (version() == X509Version::V3) {
        versionText = "v3"_el;
    }
    result.append("version"_el, versionText);
    result.append(
        "serial"_el, text::base_n::BaseNEncoder{serialNumber(), text::base_n::BaseNFormat::base16()}.toString());
    result.append("signature algorithm"_el, signatureAlgorithmId());
    result.append("issuer"_el, issuer().toString());
    result.append("valid from"_el, validFrom().toIsoString());
    result.append("valid to"_el, validTo().toIsoString());
    result.append("subject"_el, subject().toString());
    auto keyTree = text::StringTree{"Public Key"_el};
    keyTree.append("algorithm"_el, publicKey().algorithm().oid().toString());
    keyTree.append("subjectPublicKeyInfo bytes"_el, publicKey().toDer().length().toRawValue());
    result.append(keyTree);
    if (!subjectAlternativeNames().isEmpty()) {
        auto sanTree = text::StringTree{"Subject Alternative Names"_el};
        for (const auto &name : subjectAlternativeNames()) {
            auto kind = "unsupported"_el;
            auto value = text::String{};
            if (name.kind() == X509GeneralName::Kind::Dns) {
                kind = "DNS"_el;
                value = name.text();
            } else if (name.kind() == X509GeneralName::Kind::Email) {
                kind = "email"_el;
                value = name.text();
            } else if (name.kind() == X509GeneralName::Kind::Uri) {
                kind = "URI"_el;
                value = name.text();
            } else if (name.kind() == X509GeneralName::Kind::IpAddress && name.ipAddress().has_value()) {
                kind = "IP"_el;
                value = name.ipAddress()->toString();
            } else if (name.kind() == X509GeneralName::Kind::Directory) {
                kind = "directory"_el;
                value = name.directoryName().toString();
            } else if (name.kind() == X509GeneralName::Kind::RegisteredId) {
                kind = "registered ID"_el;
                value = name.registeredId().toString();
            }
            sanTree.append(kind, value);
        }
        result.append(sanTree);
    }
    if (!extensions().isEmpty()) {
        auto extensionTree = text::StringTree{"Extensions"_el};
        for (const auto &extension : extensions()) {
            auto value = text::StringEditor{};
            value.append(extension.isCritical() ? "critical, "_el : "noncritical, "_el);
            value.append(text::String::fromInteger(extension.value().length().toRawValue()));
            value.append(" bytes"_el);
            extensionTree.append(extension.oid().toString(), text::String{value});
        }
        result.append(extensionTree);
    }
    if (!profileIssues().isEmpty()) {
        auto issuesTree = text::StringTree{"Profile Issues"_el};
        for (const auto &issue : profileIssues()) {
            issuesTree.append(issue.field(), issue.diagnostic());
        }
        result.append(issuesTree);
    }
    return result;
}

void X509Certificate::writeToFile(const path::Path &path, const PemDerFormat format) const {
    if (isEmpty()) {
        throw err::LogicError{"Cannot write an empty X.509 certificate."_el};
    }
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::Certificate};
    const auto selected = file.outputFormat(format);
    if (selected == PemDerFormat::Pem) {
        file.writePem(toPem());
    } else {
        file.writeDer(toDer());
    }
}

auto X509Certificate::fromPem(const text::String &text, const X509CertificateProfileMode mode) noexcept
    -> X509Certificate {
    try {
        return fromPemOrThrow(text, mode);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto X509Certificate::fromPemOrThrow(const text::String &text, const X509CertificateProfileMode mode)
    -> X509Certificate {
    const auto certificates = impl::PemCodec{text}.decode();
    if (certificates.count() != unit::ItemCount{1U}) {
        throw err::ParseError{"A singular X509Certificate PEM factory requires exactly one certificate block."_el};
    }
    return fromDerOrThrow(certificates.first(), mode);
}

auto X509Certificate::fromDer(const mem::ByteBlock &data, const X509CertificateProfileMode mode) noexcept
    -> X509Certificate {
    try {
        return fromDerOrThrow(data, mode);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto X509Certificate::fromDerOrThrow(const mem::ByteBlock &data, const X509CertificateProfileMode mode)
    -> X509Certificate {
    return impl::X509Parser::parse(data, mode);
}

auto X509Certificate::fromFile(
    const path::Path &path, const PemDerFormat format, const X509CertificateProfileMode mode) noexcept
    -> X509Certificate {
    try {
        return fromFileOrThrow(path, format, mode);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto X509Certificate::fromFileOrThrow(
    const path::Path &path, const PemDerFormat format, const X509CertificateProfileMode mode) -> X509Certificate {
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::Certificate};
    const auto data = file.read();
    const auto selected = file.inputFormat(data, format);
    return selected == PemDerFormat::Pem ? fromPemOrThrow(impl::PemDerFileTools::toPemText(data), mode)
                                         : fromDerOrThrow(data, mode);
}

}
