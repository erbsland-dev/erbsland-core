// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PemLabel.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../util/List.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl {

/// Strict RFC 7468 PEM codec with fixed resource limits and exact artifact labels.
/// @tested{X509CertificateTest X509CertificateFileTest}
class PemCodec final {
public:
    /// Maximum PEM source length for one certificate block.
    static constexpr auto cMaximumCertificateTextLength = std::size_t{2U * 1024U * 1024U};
    /// Maximum complete bundle source length.
    static constexpr auto cMaximumBundleTextLength = std::size_t{24U * 1024U * 1024U};
    /// Maximum decoded DER length across a bundle.
    static constexpr auto cMaximumBundleDerLength = std::size_t{16U * 1024U * 1024U};
    /// Maximum number of certificates in a bundle.
    static constexpr auto cMaximumCertificates = std::size_t{256U};

public:
    /// Create a decoder for PEM source text.
    explicit PemCodec(text::String text, PemLabel label = PemLabel::Certificate) noexcept;
    /// Create an encoder for one DER certificate.
    explicit PemCodec(mem::ByteBlock certificate, PemLabel label = PemLabel::Certificate);
    /// Create an encoder for an ordered DER certificate list.
    explicit PemCodec(util::List<mem::ByteBlock> certificates, PemLabel label = PemLabel::Certificate) noexcept;

public:
    /// Decode one or more CERTIFICATE blocks.
    /// @throws err::ParseError If the textual structure or Base64 is malformed.
    /// @throws err::OutOfRangeError If a fixed resource limit is exceeded.
    [[nodiscard]] auto decode() const -> util::List<mem::ByteBlock>;
    /// Encode the configured certificate data as canonical PEM.
    [[nodiscard]] auto encode() const -> text::String;

private:
    /// Encode one certificate and append it to the result.
    void appendEncoded(text::StringEditor &result, const mem::ByteBlock &der) const;
    /// Get the exact configured encapsulation label.
    [[nodiscard]] auto labelText() const -> text::String;
    /// Build one exact pre-encapsulation boundary.
    [[nodiscard]] auto beginBoundary() const -> text::String;
    /// Build one exact post-encapsulation boundary.
    [[nodiscard]] auto endBoundary() const -> text::String;

private:
    text::String _text;                       ///< PEM source for decoding.
    util::List<mem::ByteBlock> _certificates; ///< DER certificates for encoding.
    PemLabel _label{PemLabel::Certificate};   ///< Exact artifact label.
};

}
