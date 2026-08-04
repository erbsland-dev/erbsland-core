// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509NameAttribute.hpp"

#include "../impl/X509Parser_fwd.hpp"

#include "../../util/List.hpp"

namespace erbsland::cryptology {

/// One ordered relative distinguished name in an X.509 Name.
/// @tested{X509CertificateTest}
class X509RelativeDistinguishedName final {
public:
    /// Create an empty relative distinguished name.
    X509RelativeDistinguishedName() = default;

public: // accessors
    /// Get the attributes in canonical DER order.
    [[nodiscard]] auto attributes() const noexcept -> const util::List<X509NameAttribute> & { return _attributes; }

private:
    friend class impl::X509Parser;
    /// Create a parsed relative distinguished name.
    explicit X509RelativeDistinguishedName(util::List<X509NameAttribute> attributes) noexcept :
        _attributes{std::move(attributes)} {}

private:
    util::List<X509NameAttribute> _attributes; ///< Attributes in this RDN.
};

}
