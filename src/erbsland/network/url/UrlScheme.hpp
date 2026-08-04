// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// A supported URL scheme.
enum class UrlScheme : uint8_t {
    Invalid, ///< No valid URL.
    Http,    ///< Plain HTTP.
    Https,   ///< HTTP over TLS.
    Ftp,     ///< Plain FTP.
    Ftps,    ///< FTP over TLS.
    File,    ///< A file URL.
    Mailto,  ///< An email address URL.
    Custom,  ///< A syntactically valid custom scheme.
};

}
