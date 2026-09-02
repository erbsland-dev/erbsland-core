// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509AlgorithmIdentifier.hpp"
#include "X509BasicConstraints.hpp"
#include "X509CertificateBundle_fwd.hpp"
#include "X509CertificateProfileIssue.hpp"
#include "X509CertificateProfileMode.hpp"
#include "X509Extension.hpp"
#include "X509GeneralName.hpp"
#include "X509KeyUsage.hpp"
#include "X509Name.hpp"
#include "X509Version.hpp"

#include "../asn1/Asn1Node.hpp"
#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../impl/X509CertificateBackendAccess_fwd.hpp"
#include "../impl/X509CertificateData.hpp"
#include "../impl/X509Parser_fwd.hpp"
#include "../keys/PublicKey.hpp"
#include "../PemDerFormat.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../path/Path_fwd.hpp"
#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../text/StringTree.hpp"
#include "../../time/DateTime.hpp"
#include "../../util/List.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology {

/// An immutable X.509 certificate backed by portable data or a future native certificate reference.
/// A default-constructed value is empty. `isEmpty()` describes only this storage state and does not perform signature,
/// trust-path, hostname, purpose, or validity-time validation.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509CertificateTest X509CertificateFileTest}
class X509Certificate final {
    friend class X509CertificateBundle;
    friend class impl::X509CertificateBackendAccess;
    friend class impl::X509Parser;

public:
    /// Create an empty certificate.
    X509Certificate() = default;

    // defaults
    ~X509Certificate() = default;
    X509Certificate(const X509Certificate &) noexcept = default;
    X509Certificate(X509Certificate &&) noexcept = default;
    auto operator=(const X509Certificate &) noexcept -> X509Certificate & = default;
    auto operator=(X509Certificate &&) noexcept -> X509Certificate & = default;

public: // tests
    /// Test if this facade contains no certificate.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.isNull(); }

public: // common attributes
    /// Get the X.509 version, or `Unknown` when empty.
    [[nodiscard]] auto version() const noexcept -> X509Version;
    /// Get exact serial-number INTEGER content octets, or an empty block.
    [[nodiscard]] auto serialNumber() const -> mem::ByteBlock;
    /// Get the outer signature algorithm identifier.
    [[nodiscard]] auto signatureAlgorithm() const -> X509AlgorithmIdentifier;
    /// Get the stable dotted signature algorithm identifier, or an empty string.
    [[nodiscard]] auto signatureAlgorithmId() const -> text::String;
    /// Get the issuer name.
    [[nodiscard]] auto issuer() const -> X509Name;
    /// Get the Authority Key Identifier keyIdentifier, or an empty block.
    [[nodiscard]] auto issuerId() const -> mem::ByteBlock;
    /// Get the not-before time, or an invalid date/time.
    [[nodiscard]] auto validFrom() const noexcept -> time::DateTime;
    /// Get the not-after time, or an invalid date/time.
    [[nodiscard]] auto validTo() const noexcept -> time::DateTime;
    /// Get the subject name.
    [[nodiscard]] auto subject() const -> X509Name;
    /// Get the Subject Key Identifier, or an empty block.
    [[nodiscard]] auto subjectId() const -> mem::ByteBlock;
    /// Get typed subject alternative names.
    [[nodiscard]] auto subjectAlternativeNames() const -> util::List<X509GeneralName>;
    /// Get DNS subject alternative names in encoded order.
    [[nodiscard]] auto dnsNames() const -> text::StringList;
    /// Get IP-address subject alternative names in encoded order.
    [[nodiscard]] auto ipAddresses() const -> util::List<network::IpAddress>;

public: // key and signature data
    /// Get the subject public key.
    [[nodiscard]] auto publicKey() const -> PublicKey;
    /// Get exact canonical DER for TBSCertificate.
    [[nodiscard]] auto tbsCertificateDer() const -> mem::ByteBlock;
    /// Get the TBSCertificate signature algorithm identifier.
    [[nodiscard]] auto tbsSignatureAlgorithm() const -> X509AlgorithmIdentifier;
    /// Get signature BIT STRING data without its unused-bit-count octet.
    [[nodiscard]] auto signatureData() const -> mem::ByteBlock;
    /// Get the signature BIT STRING unused-bit count.
    [[nodiscard]] auto signatureUnusedBitCount() const noexcept -> uint8_t;

public: // extensions and diagnostics
    /// Get all certificate extensions in encoded order.
    [[nodiscard]] auto extensions() const -> util::List<X509Extension>;
    /// Get decoded Basic Constraints, if present and readable.
    [[nodiscard]] auto basicConstraints() const -> std::optional<X509BasicConstraints>;
    /// Get decoded Key Usage, if present and readable.
    [[nodiscard]] auto keyUsage() const -> std::optional<X509KeyUsages>;
    /// Get decoded Extended Key Usage object identifiers.
    [[nodiscard]] auto extendedKeyUsage() const -> util::List<Asn1ObjectIdentifier>;
    /// Get retained compatible-mode profile issues.
    [[nodiscard]] auto profileIssues() const -> util::List<X509CertificateProfileIssue>;
    /// Get the complete read-only ASN.1 root node.
    [[nodiscard]] auto asn1() const -> Asn1Node;

public: // conversion
    /// Encode this certificate using strict RFC 7468 textual form, or return an empty string when empty.
    [[nodiscard]] auto toPem() const -> text::String;
    /// Return exact canonical DER, or an empty block when empty.
    [[nodiscard]] auto toDer() const -> mem::ByteBlock;
    /// Return a deterministic one-line subject, issuer, and serial summary.
    [[nodiscard]] auto toString() const -> text::String;
    /// Build a complete OpenSSL-like certificate display tree.
    [[nodiscard]] auto toStringTree() const -> text::StringTree;
    /// Write this certificate to a file.
    /// @param path The destination path.
    /// @param format The explicit format or suffix-selected `Automatic` format.
    /// @throws err::LogicError If the certificate is empty.
    /// @throws err::ParameterError If automatic output cannot select a format.
    /// @throws path::PathError If writing fails.
    void writeToFile(const path::Path &path, PemDerFormat format = PemDerFormat::Automatic) const;

public: // factories
    /// Parse exactly one strict CERTIFICATE PEM block, returning an empty certificate on any error.
    [[nodiscard]] static auto fromPem(
        const text::String &text, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) noexcept
        -> X509Certificate;
    /// Parse exactly one strict CERTIFICATE PEM block.
    /// @throws err::ParseError If textual or certificate data is malformed or rejected by the selected profile mode.
    /// @throws err::OutOfRangeError If a fixed resource limit is exceeded.
    [[nodiscard]] static auto fromPemOrThrow(
        const text::String &text, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict)
        -> X509Certificate;
    /// Parse exactly one complete canonical DER certificate, returning an empty certificate on any error.
    [[nodiscard]] static auto fromDer(
        const mem::ByteBlock &data, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) noexcept
        -> X509Certificate;
    /// Parse exactly one complete canonical DER certificate.
    /// @throws err::ParseError If DER or certificate data is malformed or rejected by the selected profile mode.
    /// @throws err::OutOfRangeError If a fixed resource limit is exceeded.
    [[nodiscard]] static auto fromDerOrThrow(
        const mem::ByteBlock &data, X509CertificateProfileMode mode = X509CertificateProfileMode::Strict)
        -> X509Certificate;
    /// Read one certificate from a file, returning an empty certificate on any error.
    [[nodiscard]] static auto fromFile(
        const path::Path &path,
        PemDerFormat format = PemDerFormat::Automatic,
        X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) noexcept -> X509Certificate;
    /// Read one certificate from a file.
    /// @throws path::PathError If reading fails.
    /// @throws err::ParseError If certificate data is malformed.
    /// @throws err::OutOfRangeError If a fixed resource limit is exceeded.
    [[nodiscard]] static auto fromFileOrThrow(
        const path::Path &path,
        PemDerFormat format = PemDerFormat::Automatic,
        X509CertificateProfileMode mode = X509CertificateProfileMode::Strict) -> X509Certificate;

private:
    /// Create a certificate retaining implementation data.
    /// @param data The implementation data to retain.
    explicit X509Certificate(impl::X509CertificateDataPtr data) noexcept : _data{std::move(data)} {}
    /// Access portable certificate data, or `nullptr` when this certificate uses another representation.
    [[nodiscard]] auto portableData() const -> const impl::PortableX509CertificateData *;

private:
    impl::X509CertificateDataPtr _data; ///< Polymorphic shared certificate data.
};

}
