// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509CertificateSigningRequest_fwd.hpp"

#include "../impl/X509ArtifactWriter_fwd.hpp"
#include "../PemDerFormat.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../path/Path_fwd.hpp"
#include "../../text/String_fwd.hpp"

namespace erbsland::cryptology {

/// An immutable PKCS#10 certificate signing request.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509CertificateBuilderTest}
class X509CertificateSigningRequest final {
public:
    /// Create an empty request.
    X509CertificateSigningRequest() = default;

public:
    /// Test if no request is stored.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _der.isEmpty(); }
    /// Return canonical PKCS#10 DER.
    [[nodiscard]] auto toDer() const noexcept -> const mem::ByteBlock & { return _der; }
    /// Return one `CERTIFICATE REQUEST` PEM block.
    [[nodiscard]] auto toPem() const -> text::String;
    /// Write the request without replacing an existing file.
    void writeToFile(const path::Path &path, PemDerFormat format = PemDerFormat::Automatic) const;

private:
    friend class impl::X509ArtifactWriter;
    /// Create a request from validated canonical DER.
    explicit X509CertificateSigningRequest(mem::ByteBlock der) noexcept : _der{std::move(der)} {}

private:
    mem::ByteBlock _der;
};

}
