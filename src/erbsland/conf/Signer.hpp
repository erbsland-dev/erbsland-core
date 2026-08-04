// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SignatureSigner_fwd.hpp"

#include "../path/Path.hpp"

namespace erbsland::conf {

/// The tool to sign configuration documents.
/// @tested{ParserSignatureTest}
class Signer final {
public:
    /// Create a new signer tool using the given implementation.
    /// @param signatureSigner The signature signer implementation to use.
    explicit Signer(SignatureSignerPtr signatureSigner);

    // defaults
    ~Signer() = default;

public:
    /// Sign a document.
    /// This signs a given document. The signed document is not parsed, and therefore its syntax is not checked.
    /// It is recommended that you use `Parser` to verify the document before signing it.
    /// - The encoding of the document is checked, as UTF-8 is fully decoded/encoded.
    /// - The line lengths are checked, as the document is read line-by-line.
    /// - An existing initial `\@signature` line is skipped and replaced in the destination.
    /// @param sourcePath The path of the document to sign.
    /// @param destinationPath The path where the signed document is stored.
    /// @param signingPersonText The text identifying the signing person.
    /// @throws ConfError (IO, Encoding) in case of any problem with the signing process.
    void sign(path::Path sourcePath, path::Path destinationPath, text::String signingPersonText);

private:
    SignatureSignerPtr _signatureSigner;
};

}
