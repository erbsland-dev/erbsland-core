// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../x509/X509CertificateFormat.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../path/Path.hpp"
#include "../../text/String_fwd.hpp"

namespace erbsland::cryptology::impl {

/// Shared certificate file-format detection and bounded content I/O.
/// @tested{X509CertificateFileTest}
class X509FileTools final {
public:
    /// Create file tools bound to one certificate path.
    explicit X509FileTools(path::Path path) noexcept;

public:
    /// Read bounded file bytes.
    [[nodiscard]] auto read() const -> mem::ByteBlock;
    /// Select the input format from an explicit request, strong suffix hint, or content sniffing.
    [[nodiscard]] auto inputFormat(const mem::ByteBlock &data, X509CertificateFormat requested) const
        -> X509CertificateFormat;
    /// Select output format from an explicit request or recognized suffix.
    [[nodiscard]] auto outputFormat(X509CertificateFormat requested) const -> X509CertificateFormat;
    /// Decode UTF-8 PEM source bytes and reject a BOM or malformed encoding.
    [[nodiscard]] static auto toPemText(const mem::ByteBlock &data) -> text::String;
    /// Write canonical PEM text.
    void writePem(const text::String &text) const;
    /// Write DER bytes.
    void writeDer(const mem::ByteBlock &data) const;

private:
    /// Test the bound path suffix with ASCII case folding.
    [[nodiscard]] auto suffixIs(const text::String &suffix) const noexcept -> bool;
    /// Detect an RFC 7468 CERTIFICATE pre-encapsulation boundary after leading whitespace.
    [[nodiscard]] static auto looksLikePem(const mem::ByteBlock &data) noexcept -> bool;

private:
    path::Path _path; ///< The certificate path used for I/O and suffix detection.
};

}
