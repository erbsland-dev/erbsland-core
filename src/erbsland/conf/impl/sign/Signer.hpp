// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../path/Path.hpp"
#include "../../../text/String.hpp"
#include "../../SignatureSigner_fwd.hpp"
#include "../../Source_fwd.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

/// The implementation of the signer tool.
class Signer final {
public:
    /// Create a signer using a signature backend.
    /// @param signatureSigner The backend used to create signatures.
    explicit Signer(SignatureSignerPtr signatureSigner);

    // defaults
    ~Signer() = default;

public:
    /// Sign a source configuration file and write the signed result.
    void sign(path::Path sourcePath, path::Path destinationPath, text::String signingPersonText);

private:
    /// Stores the canonical digest and line-ending information for signing.
    struct DigestResult final {
        text::String digestText;
        mem::ByteBlock digest;
        bool hasWindowsLineEndings;
    };

    /// Validate a source path and create its configuration source.
    auto validateAndCreateSource(path::Path sourcePath) -> SourcePtr;

    /// Build the digest for a configuration source.
    auto buildDigest(const SourcePtr &source) -> DigestResult;

    /// Validate and escape signature metadata text.
    void validateAndEscapeSignatureText(text::String &signatureText);

    /// Write the signed configuration file.
    void writeSignedFile(
        const path::Path &sourcePath,
        const path::Path &destinationPath,
        const text::String &digestText,
        const mem::ByteBlock &digest,
        bool hasWindowsLineBreaks);

    /// Write canonical configuration data into output.
    auto writeConfiguration(text::StringEditor &output, const path::Path &sourcePath) -> mem::ByteBlock;

private:
    SignatureSignerPtr _signatureSigner;
};

}
