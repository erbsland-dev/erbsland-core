// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1Node.hpp"
#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../impl/X509Parser_fwd.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::cryptology {

/// An X.509 AlgorithmIdentifier preserving its parameters and exact DER.
/// @tested{EcdsaSignatureTest RsaSignatureTest X509CertificateTest}
class X509AlgorithmIdentifier final {
    friend class impl::X509Parser;

public:
    /// Create an empty algorithm identifier.
    X509AlgorithmIdentifier() = default;

    // defaults
    ~X509AlgorithmIdentifier() = default;
    X509AlgorithmIdentifier(const X509AlgorithmIdentifier &) = default;
    X509AlgorithmIdentifier(X509AlgorithmIdentifier &&) noexcept = default;
    auto operator=(const X509AlgorithmIdentifier &) -> X509AlgorithmIdentifier & = default;
    auto operator=(X509AlgorithmIdentifier &&) noexcept -> X509AlgorithmIdentifier & = default;

public: // tests
    /// Test if this identifier is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _oid.isEmpty(); }

public: // accessors
    /// Get the algorithm object identifier.
    [[nodiscard]] auto oid() const noexcept -> const Asn1ObjectIdentifier & { return _oid; }
    /// Get the optional parameters node.
    [[nodiscard]] auto parameters() const noexcept -> const Asn1Node & { return _parameters; }
    /// Get the complete exact DER encoding.
    [[nodiscard]] auto toDer() const noexcept -> const mem::ByteBlock & { return _der; }

public: // factories
    /// Parse one canonical DER AlgorithmIdentifier, returning an empty value on error.
    /// @param der The complete DER AlgorithmIdentifier.
    /// @return The parsed identifier, or an empty value on error.
    [[nodiscard]] static auto fromDer(const mem::ByteBlock &der) noexcept -> X509AlgorithmIdentifier;
    /// Parse one canonical DER AlgorithmIdentifier.
    /// @param der The complete DER AlgorithmIdentifier.
    /// @return The parsed identifier.
    /// @throws err::ParseError If the DER or AlgorithmIdentifier structure is malformed.
    /// @throws err::OutOfRangeError If a fixed parser resource limit is exceeded.
    [[nodiscard]] static auto fromDerOrThrow(const mem::ByteBlock &der) -> X509AlgorithmIdentifier;

private:
    /// Create an algorithm identifier from parsed DER components.
    X509AlgorithmIdentifier(Asn1ObjectIdentifier oid, Asn1Node parameters, mem::ByteBlock der) noexcept :
        _oid{std::move(oid)}, _parameters{std::move(parameters)}, _der{std::move(der)} {}

private:
    Asn1ObjectIdentifier _oid; ///< Algorithm OID.
    Asn1Node _parameters;      ///< Optional parameters.
    mem::ByteBlock _der;       ///< Complete exact DER.
};

}
