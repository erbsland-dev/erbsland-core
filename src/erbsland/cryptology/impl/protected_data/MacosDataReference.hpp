// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __APPLE__
#error "This header is only available on macOS."
#endif

#include <Security/Security.h>

namespace erbsland::cryptology::impl {

/// Own one Core Foundation data reference.
/// @notest{Exercised through the macOS protected-data integration test.}
class MacosDataReference final {
public:
    /// Take ownership of a returned data reference.
    explicit MacosDataReference(CFDataRef data) noexcept : _data{data} {}
    /// Release the reference.
    ~MacosDataReference() {
        if (_data != nullptr) {
            CFRelease(_data);
        }
    }

    // defaults/deletions
    MacosDataReference(const MacosDataReference &) = delete;
    MacosDataReference(MacosDataReference &&) = delete;
    auto operator=(const MacosDataReference &) -> MacosDataReference & = delete;
    auto operator=(MacosDataReference &&) -> MacosDataReference & = delete;

public: // accessors
    /// Get the owned Core Foundation data reference.
    [[nodiscard]] auto get() const noexcept -> CFDataRef { return _data; }
    /// Test whether this wrapper owns no Core Foundation data reference.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data == nullptr; }

private:
    CFDataRef _data{nullptr}; ///< Owned reference.
};

}
