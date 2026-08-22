// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceErrorCategory.hpp"

#include "../err/RuntimeError.hpp"

namespace erbsland::resource {

/// A required resource lookup or decoding failure.
/// @tested{ResourceManagerTest}
class ResourceError final : public err::RuntimeError {
public:
    /// Create a resource error.
    ResourceError(ResourceErrorCategory category, text::String message) noexcept;
    /// @overload
    ResourceError(ResourceErrorCategory category, std::string_view message) noexcept;

    // defaults
    ~ResourceError() override = default;

public:
    /// Get the machine-readable error category.
    [[nodiscard]] auto category() const noexcept -> ResourceErrorCategory { return _category; }

private:
    ResourceErrorCategory _category; ///< The machine-readable error category.
};

}
