#pragma once

#include "../x509/X509Certificate.hpp"

namespace erbsland::cryptology {

/// Public metadata identifying one current-user certificate without exposing a native handle.
/// @tested{SystemIdentityStoreTest}
struct SystemIdentityInfo {
    mem::ByteBlock fingerprint;  ///< Exact SHA-256 of the leaf certificate DER, used for unambiguous selection.
    X509Certificate certificate; ///< Public leaf certificate; listing does not sign or export a private key.
};

}
