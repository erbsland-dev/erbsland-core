// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::cryptology::impl {
class PasswordHashData;
/// A shared pointer to immutable parsed password-hash data.
using PasswordHashDataPtr = std::shared_ptr<PasswordHashData>;
}
