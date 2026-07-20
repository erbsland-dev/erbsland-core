// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceIdentifier.hpp"

namespace erbsland::conf {

/// The data for the signer implementation.
struct SignatureSignerData final {
    /// The source identifier of the document.
    SourceIdentifierPtr sourceIdentifier;
    /// The raw and unprocessed text for the signer that was passed to the `sign` method.
    text::String signingPersonText;
    /// The cryptographic hash of the document.
    /// The hash always has the format `<type> <hash as a lowercase hex byte sequence>`. As the responsibility of the
    /// application is only the verification of the signature, decoding of this text shouldn't be necessary. Instead,
    /// the application should compare and sign/verify this text as it is.
    text::String documentDigest;
};

}
