// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class SignatureValidator;
/// Shared pointer for SignatureValidator.
using SignatureValidatorPtr = std::shared_ptr<SignatureValidator>;

}
