// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1Node.hpp"
#include "../keys/PublicKey.hpp"
#include "../x509/X509AlgorithmIdentifier.hpp"
#include "../x509/X509BasicConstraints.hpp"
#include "../x509/X509CertificateProfileIssue.hpp"
#include "../x509/X509Extension.hpp"
#include "../x509/X509GeneralName.hpp"
#include "../x509/X509KeyUsage.hpp"
#include "../x509/X509Name.hpp"
#include "../x509/X509Version.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../time/DateTime.hpp"
#include "../../util/List.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology::impl {

/// Complete parsed X.509 certificate values.
/// @tested{X509CertificateTest X509CertificateFileTest}
struct PortableX509CertificateValues final {
    Asn1Node root;                                         ///< Certificate ASN.1 root.
    Asn1Node tbsNode;                                      ///< TBSCertificate node.
    X509Version version{X509Version::Unknown};             ///< Certificate version.
    mem::ByteBlock serialNumber;                           ///< Exact serial INTEGER contents.
    X509AlgorithmIdentifier tbsSignatureAlgorithm;         ///< TBS signature algorithm.
    X509AlgorithmIdentifier signatureAlgorithm;            ///< Outer signature algorithm.
    mem::ByteBlock signature;                              ///< Signature BIT STRING data.
    uint8_t signatureUnusedBitCount{};                     ///< Signature unused bits.
    X509Name issuer;                                       ///< Issuer name.
    time::DateTime validFrom;                              ///< UTC not-before time.
    time::DateTime validTo;                                ///< UTC not-after time.
    X509Name subject;                                      ///< Subject name.
    PublicKey publicKey;                                   ///< Subject public key.
    util::List<X509Extension> extensions;                  ///< All extensions.
    util::List<X509GeneralName> subjectAlternativeNames;   ///< Decoded SAN entries.
    mem::ByteBlock subjectKeyIdentifier;                   ///< Subject Key Identifier.
    mem::ByteBlock authorityKeyIdentifier;                 ///< Authority Key Identifier keyIdentifier.
    std::optional<X509BasicConstraints> basicConstraints;  ///< Basic Constraints.
    std::optional<X509KeyUsages> keyUsage;                 ///< Key Usage.
    util::List<Asn1ObjectIdentifier> extendedKeyUsage;     ///< Extended Key Usage OIDs.
    util::List<X509CertificateProfileIssue> profileIssues; ///< Retained compatible-mode issues.
};

}
