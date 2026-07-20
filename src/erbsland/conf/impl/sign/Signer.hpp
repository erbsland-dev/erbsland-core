// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../path/Path.hpp"
#include "../../../text/String.hpp"
#include "../../SignatureSigner.hpp"
#include "../../Source_fwd.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

/// The implementation of the signer tool.
class Signer final {
public:
    explicit Signer(SignatureSignerPtr signatureSigner);

    // defaults
    ~Signer() = default;

public:
    void sign(path::Path sourcePath, path::Path destinationPath, text::String signingPersonText);

private:
    struct DigestResult final {
        text::String digestText;
        mem::ByteBlock digest;
        bool hasWindowsLineEndings;
    };

    auto validateAndCreateSource(path::Path sourcePath) -> SourcePtr;

    auto buildDigest(const SourcePtr &source) -> DigestResult;

    void validateAndEscapeSignatureText(text::String &signatureText);

    void writeSignedFile(
        const path::Path &sourcePath,
        const path::Path &destinationPath,
        const text::String &digestText,
        const mem::ByteBlock &digest,
        bool hasWindowsLineBreaks);

    auto writeConfiguration(text::StringEditor &output, const path::Path &sourcePath) -> mem::ByteBlock;

private:
    SignatureSignerPtr _signatureSigner;
};

}
