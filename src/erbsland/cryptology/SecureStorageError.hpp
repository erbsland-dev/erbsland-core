#pragma once

#include "SecureStorageErrorCategory.hpp"

#include "../err/RuntimeError.hpp"

#include <utility>

namespace erbsland::cryptology {

/// A persistent secret failure without secret contents in its diagnostics.
/// @tested{SecureStorageTest}
class SecureStorageError final : public err::RuntimeError {
public:
    /// Create a categorized error.
    /// @param category The failure category.
    /// @param reason The non-secret diagnostic text.
    SecureStorageError(SecureStorageErrorCategory category, text::String reason) :
        err::RuntimeError{std::move(reason)}, _category{category} {}

public: // accessors
    /// Get the failure category.
    [[nodiscard]] auto category() const noexcept -> SecureStorageErrorCategory { return _category; }

private:
    SecureStorageErrorCategory _category; ///< Stable failure category.
};

}
