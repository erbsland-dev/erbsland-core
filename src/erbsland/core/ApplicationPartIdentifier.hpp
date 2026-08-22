// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartIdentifier_fwd.hpp"

#include "../text/String.hpp"

namespace erbsland::core {

/// A stable, named identifier for an application part.
/// Identifier names are authoritative. Implementations may cache manager-local lookup information without exposing
/// mutable state through this interface.
/// @seedoc{/reference/core/application_parts}
/// @tested{ApplicationPartManagerTest}
class ApplicationPartIdentifier {
public:
    // defaults
    virtual ~ApplicationPartIdentifier() = default;

public:
    /// Get the stable identifier name.
    [[nodiscard]] virtual auto name() const noexcept -> const text::String & = 0;
    /// Convert this identifier to text.
    [[nodiscard]] auto toString() const noexcept -> text::String { return name(); }

public: // factory methods
    /// Create an application-part identifier.
    /// @param name A non-empty ASCII reverse-domain token using letters, digits, `.`, `_`, and `-`.
    /// @return A new identifier with no manager-local cache.
    /// @throws err::ParameterError If `name` is invalid or longer than 200 characters.
    [[nodiscard]] static auto create(text::String name) -> ApplicationPartIdentifierPtr;
};

}
