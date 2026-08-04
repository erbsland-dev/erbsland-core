// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Asn1Node_fwd.hpp"

#include "../impl/X509Parser_fwd.hpp"
#include "../x509/X509Certificate_fwd.hpp"
#include "../x509/X509Name_fwd.hpp"

#include "../../text/String.hpp"

#include <compare>
#include <optional>

namespace erbsland::cryptology {

/// A canonical dotted ASN.1 object identifier.
/// @tested{Asn1NodeTest X509CertificateTest}
class Asn1ObjectIdentifier final {
    friend class Asn1Node;
    friend class X509Certificate;
    friend class X509Name;
    friend class impl::X509Parser;

public:
    /// Create an empty object identifier.
    Asn1ObjectIdentifier() = default;

    // defaults
    ~Asn1ObjectIdentifier() = default;
    Asn1ObjectIdentifier(const Asn1ObjectIdentifier &) = default;
    Asn1ObjectIdentifier(Asn1ObjectIdentifier &&) noexcept = default;
    auto operator=(const Asn1ObjectIdentifier &) -> Asn1ObjectIdentifier & = default;
    auto operator=(Asn1ObjectIdentifier &&) noexcept -> Asn1ObjectIdentifier & = default;

public: // operators
    [[nodiscard]] auto operator<=>(const Asn1ObjectIdentifier &other) const noexcept -> std::strong_ordering = default;

public: // tests
    /// Test if this object identifier is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _text.isEmpty(); }

public: // conversion
    /// Return the canonical dotted-decimal representation.
    [[nodiscard]] auto toString() const noexcept -> const text::String & { return _text; }
    /// Parse a canonical dotted-decimal object identifier.
    /// @param value The text to parse.
    /// @return The parsed identifier, or no value for malformed text.
    [[nodiscard]] static auto fromString(const text::String &value) noexcept -> std::optional<Asn1ObjectIdentifier>;
    /// Parse a canonical dotted-decimal object identifier.
    /// @param value The text to parse.
    /// @return The parsed identifier.
    /// @throws err::ParseError If the value is malformed or noncanonical.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &value) -> Asn1ObjectIdentifier;

private:
    /// Create an object identifier from validated text.
    /// @param value The object identifier text to retain.
    explicit Asn1ObjectIdentifier(text::String value) noexcept : _text{std::move(value)} {}

private:
    text::String _text; ///< The canonical dotted-decimal representation.
};

}
