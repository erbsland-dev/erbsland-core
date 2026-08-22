// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ApplicationPartIdentifier.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace erbsland::core::impl {

/// The application-part identifier implementation with one manager-local lookup cache.
/// @tested{ApplicationPartManagerTest}
class ApplicationPartIdentifier final : public core::ApplicationPartIdentifier {
public:
    /// Create an identifier implementation.
    explicit ApplicationPartIdentifier(text::String name) noexcept : _name{std::move(name)} {}

public: // implement core::ApplicationPartIdentifier
    [[nodiscard]] auto name() const noexcept -> const text::String & override { return _name; }

public: // internal cache
    /// Resolve a cached number for the given manager token.
    [[nodiscard]] auto cachedNumber(uint64_t managerToken) const noexcept -> std::size_t;
    /// Cache a manager-local number.
    void setCachedNumber(uint64_t managerToken, std::size_t number) noexcept;

private:
    text::String _name;              ///< The stable identifier name.
    mutable std::mutex _cacheMutex;  ///< Protects the manager-local cache.
    uint64_t _cachedManagerToken{0}; ///< The manager that assigned the cached number.
    std::size_t _cachedNumber{0};    ///< The cached manager-local number, or zero.
};

}
