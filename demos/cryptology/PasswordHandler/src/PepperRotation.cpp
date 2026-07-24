// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PepperRotation.hpp"

namespace demo {

/// Configure active and fallback peppers for gradual password-record migration.
///
/// The active key has a public identifier stored in new password records. Historical keys verify existing records.
/// There may be at most one unnamed fallback for legacy hashes. A successful fallback verification returns a
/// replacement hash that the login command persists before reporting success.
auto buildPasswordHasher(const el::PasswordHashKey &activeKey, const el::List<el::PasswordHashKey> &fallbackKeys)
    -> el::PasswordHasher {
    return el::PasswordHasher::withKeyRotation(activeKey, fallbackKeys);
}

}
