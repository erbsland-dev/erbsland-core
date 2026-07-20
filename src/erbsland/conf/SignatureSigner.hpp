// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SignatureSignerData.hpp"

namespace erbsland::conf {

class SignatureSigner;
using SignatureSignerPtr = std::shared_ptr<SignatureSigner>;

/// The signer interface to create new signatures when signing documents.
class SignatureSigner {
public:
    /// Default destructor.
    virtual ~SignatureSigner() = default;

public:
    /// Create the signature text when signing a document.
    /// @param data The data from the document to create the signature from.
    /// @return The text that shall be added to the `\@signature` meta-value in the stored document.
    ///     This text must be shorter than 3980 *bytes* to fit into a single line of the configuration.
    [[nodiscard]] virtual auto sign(const SignatureSignerData &data) -> text::String = 0;
};

}
