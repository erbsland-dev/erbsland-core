// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../util/List.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl {

/// Strict RFC 7468 certificate PEM codec with fixed resource limits.
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
    explicit PemCodec(text::String text) noexcept;
    /// Create an encoder for one DER certificate.
    explicit PemCodec(mem::ByteBlock certificate);
    /// Create an encoder for an ordered DER certificate list.
    explicit PemCodec(util::List<mem::ByteBlock> certificates) noexcept;

public:
    /// Decode one or more CERTIFICATE blocks.
    /// @throws err::ParseError If the textual structure or Base64 is malformed.
    /// @throws err::OutOfRangeError If a fixed resource limit is exceeded.
    [[nodiscard]] auto decode() const -> util::List<mem::ByteBlock>;
    /// Encode the configured certificate data as canonical PEM.
    [[nodiscard]] auto encode() const -> text::String;

private:
    /// Encode one certificate and append it to the result.
    static void appendEncoded(text::StringEditor &result, const mem::ByteBlock &der);

private:
    text::String _text;                       ///< PEM source for decoding.
    util::List<mem::ByteBlock> _certificates; ///< DER certificates for encoding.
};

}
