// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class SignatureSigner;
/// Shared pointer for SignatureSigner.
using SignatureSignerPtr = std::shared_ptr<SignatureSigner>;

}
