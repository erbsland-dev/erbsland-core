// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PemDerFormat.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../path/Path.hpp"
#include "../../text/String_fwd.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

/// Shared bounded file I/O and suffix selection for DER-based cryptographic artifacts.
/// @tested{SigningPrivateKeyTest X509CertificateFileTest}
class PemDerFileTools final {
public:
    /// The artifact whose conventional suffixes are recognized.
    enum class Artifact : uint8_t {
        Certificate,
        PrivateKey,
        PublicKey,
        CertificateRequest,
    };

public:
    /// Bind the helper to one path and artifact kind.
    PemDerFileTools(path::Path path, Artifact artifact, bool sensitive = false) noexcept;

public:
    /// Read one bounded artifact file.
    [[nodiscard]] auto read() const -> mem::ByteBlock;
    /// Select an input format from the request, suffix, or content.
    [[nodiscard]] auto inputFormat(const mem::ByteBlock &data, PemDerFormat requested) const -> PemDerFormat;
    /// Select an output format from the request or suffix.
    [[nodiscard]] auto outputFormat(PemDerFormat requested) const -> PemDerFormat;
    /// Decode strict BOM-free UTF-8 PEM text.
    [[nodiscard]] static auto toPemText(const mem::ByteBlock &data) -> text::String;
    /// Write canonical PEM text without replacing an existing file.
    void writePem(const text::String &text) const;
    /// Write DER bytes without replacing an existing file.
    void writeDer(const mem::ByteBlock &data) const;

private:
    /// Test one suffix using ASCII case folding.
    [[nodiscard]] auto suffixIs(const text::String &suffix) const noexcept -> bool;
    /// Select the artifact-specific format encoded by the bound path suffix.
    [[nodiscard]] auto suffixFormat() const noexcept -> PemDerFormat;
    /// Test whether bounded input begins with a PEM boundary after ASCII whitespace.
    [[nodiscard]] static auto looksLikePem(const mem::ByteBlock &data) noexcept -> bool;

private:
    path::Path _path;
    Artifact _artifact;
    bool _sensitive;
};

}
