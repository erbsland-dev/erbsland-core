// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProtectedDataAccess_fwd.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteLength.hpp"

#include <atomic>

namespace erbsland::cryptology::impl {

/// Resolve protected-data operations without storing provider references in protected blocks.
/// @notest{Internal interface exercised through ProtectedByteBlockTest.}
class ProtectedDataAccess {
public:
    // defaults
    virtual ~ProtectedDataAccess() = default;

public:
    /// Get the application or test protected-data access implementation.
    [[nodiscard]] static auto access() -> ProtectedDataAccess &;
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    /// Replace access resolution for tests. A null pointer restores application access.
    static void setTestAccess(ProtectedDataAccess *access) noexcept;
#endif

public:
    /// Encrypt plaintext into a protected envelope.
    [[nodiscard]] virtual auto protect(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength)
        -> mem::ByteBlock = 0;
    /// Decrypt a protected envelope into plaintext.
    [[nodiscard]] virtual auto unprotect(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength)
        -> mem::ByteBlock = 0;

private:
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    static std::atomic<ProtectedDataAccess *> _testAccess; ///< Optional access override for tests.
#endif
};

}
