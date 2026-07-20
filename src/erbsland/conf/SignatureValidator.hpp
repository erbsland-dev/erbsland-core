// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SignatureValidatorData.hpp"
#include "SignatureValidatorResult.hpp"

#include <memory>

namespace erbsland::conf {

class SignatureValidator;
using SignatureValidatorPtr = std::shared_ptr<SignatureValidator>;

/// The interface for signature validation.
class SignatureValidator {
public:
    /// Default destructor.
    virtual ~SignatureValidator() = default;

public:
    /// The method for validating the signature.
    /// The parser calls this method to validate the signature of a document.
    /// The function is called for *every document*, no matter if it has a signature or not.
    /// @param data The data with the details to validate the signature.
    /// @return The validation result.
    [[nodiscard]] virtual auto validate(const SignatureValidatorData &data) -> SignatureValidatorResult = 0;
};

}
