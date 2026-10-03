#pragma once

#include "../MacosOwnedReference.hpp"

#include <Security/Security.h>

namespace erbsland::cryptology::impl {

/// Own every Core Foundation object retained by one native trust evaluation.
/// @needtest{Native trust failure cleanup.}
struct MacosTrustResources final {
    MacosOwnedReference<CFMutableArrayRef> certificates{}; ///< Owned peer certificate array.
    MacosOwnedReference<SecPolicyRef> sslPolicy{};         ///< Owned TLS server policy.
    MacosOwnedReference<SecPolicyRef> revocationPolicy{};  ///< Owned best-effort revocation policy.
    MacosOwnedReference<CFArrayRef> policies{};            ///< Owned policy array.
    MacosOwnedReference<CFDateRef> date{};                 ///< Owned explicit validation time.
    MacosOwnedReference<SecTrustRef> trust{};              ///< Owned trust evaluator.
    MacosOwnedReference<CFErrorRef> error{};               ///< Owned evaluation error.
    MacosOwnedReference<CFArrayRef> chain{};               ///< Owned selected certificate path.
    MacosOwnedReference<CFDictionaryRef> result{};         ///< Owned revocation result metadata.
};

}
