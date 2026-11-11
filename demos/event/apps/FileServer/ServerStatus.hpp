// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ApplicationPartIdentifier.hpp>
#include <erbsland/Literals.hpp>

#include <cstddef>

namespace demo {

using namespace el::text::literals;

/// Public statistics interface for the file-server part.
/// Callers may read a snapshot from any thread without accessing the HTTP source.
class ServerStatus {
public:
    // defaults
    virtual ~ServerStatus() = default;

public: // registration
    /// Get the stable identifier shared by the interface and its application part.
    [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
        static const auto identifier = el::ApplicationPartIdentifier::create("dev.erbsland.demo.file-server"_el);
        return identifier;
    }

public: // statistics
    /// Get the number of directory-index requests received so far, from any thread.
    [[nodiscard]] virtual auto indexRequestCount() const -> std::size_t = 0;
};

}
