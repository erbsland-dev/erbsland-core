// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::system {

/// A platform-neutral category for a native operating-system error.
/// @tested{DiagnosticTest}
class PlatformErrorCategory final {
public:
    /// Portable native failure categories.
    enum Value : std::uint8_t {
        Unknown,
        NotFound,
        PermissionDenied,
        AlreadyExists,
        InvalidPath,
        NotDirectory,
        IsDirectory,
        NameTooLong,
        SymbolicLinkLoop,
        ReadOnlyFileSystem,
        StorageFull,
        QuotaExceeded,
        CrossDevice,
        ResourceBusy,
        TooManyOpenFiles,
        FileTooLarge,
        Unsupported,
    };

public:
    /// Create a category from its raw value.
    constexpr PlatformErrorCategory(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr PlatformErrorCategory() noexcept = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const PlatformErrorCategory &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const PlatformErrorCategory &other, value, other._value);

public: // accessors
    /// Get the raw category value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Get the stable category identifier.
    [[nodiscard]] auto toString() const -> text::StringView;

private:
    Value _value{Unknown};
};

}
