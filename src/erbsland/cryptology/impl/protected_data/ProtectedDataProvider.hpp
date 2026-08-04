// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProtectedDataProvider_fwd.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteLength.hpp"

#include <atomic>

namespace erbsland::cryptology::impl {

/// Backend interface for application-scoped protected byte blocks.
/// @tested{ProtectedByteBlockTest}
class ProtectedDataProvider {
public:
    // defaults/deletions
    ProtectedDataProvider() = default;
    virtual ~ProtectedDataProvider() = default;
    ProtectedDataProvider(const ProtectedDataProvider &) = delete;
    ProtectedDataProvider(ProtectedDataProvider &&) = delete;
    auto operator=(const ProtectedDataProvider &) -> ProtectedDataProvider & = delete;
    auto operator=(ProtectedDataProvider &&) -> ProtectedDataProvider & = delete;

public:
    /// Create the bundled internal AES-256-GCM provider.
    [[nodiscard]] static auto createInternal() -> ProtectedDataProviderPtr;
    /// Create the native platform provider, or an empty pointer when this platform has none.
    [[nodiscard]] static auto createPlatform() -> ProtectedDataProviderPtr;
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    /// Test factory function for replacing native-provider creation.
    using TestPlatformFactory = auto (*)() -> ProtectedDataProviderPtr;
    /// Replace native-provider creation for tests. A null pointer restores platform behavior.
    static void setTestPlatformFactory(TestPlatformFactory factory) noexcept;
#endif

public:
    /// Encrypt and authenticate plaintext into a provider-owned envelope.
    [[nodiscard]] virtual auto protect(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength)
        -> mem::ByteBlock = 0;
    /// Authenticate and decrypt a provider-owned envelope.
    [[nodiscard]] virtual auto unprotect(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength)
        -> mem::ByteBlock = 0;

private:
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    static std::atomic<TestPlatformFactory> _testPlatformFactory; ///< Optional platform factory override for tests.
#endif
};

}
