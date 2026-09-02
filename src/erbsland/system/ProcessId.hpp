// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessId_fwd.hpp"

#include "impl/ProcessIdAccess_fwd.hpp"

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace erbsland::system {

/// A platform-independent process identifier.
/// A valid identifier can be compared and passed to process APIs, but does not guarantee that a process currently
/// exists for it. Only system APIs create valid identifiers.
/// @seedoc{/reference/system/system_services}
/// @tested{ProcessInfoTest SubprocessInteropTest}
class ProcessId final {
    friend class impl::ProcessIdAccess;

public:
    /// Create an invalid process identifier.
    constexpr ProcessId() noexcept = default;

    // defaults
    ~ProcessId() = default;
    ProcessId(const ProcessId &) noexcept = default;
    ProcessId(ProcessId &&) noexcept = default;
    auto operator=(const ProcessId &) noexcept -> ProcessId & = default;
    auto operator=(ProcessId &&) noexcept -> ProcessId & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ProcessId &other, other._value);

public: // tests
    /// Test if this process identifier can represent a native process.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _value != 0U; }

public: // conversion
    /// Convert the identifier to decimal text, or return empty text for an invalid identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get a stable hash for this identifier.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t { return std::hash<std::uint64_t>{}(_value); }

private:
    /// Create a valid identifier from its native numeric representation.
    explicit constexpr ProcessId(const std::uint64_t value) noexcept : _value{value} {}

private:
    std::uint64_t _value{}; ///< Opaque native process identifier.
};

}

template <>
struct std::hash<erbsland::system::ProcessId> {
    auto operator()(const erbsland::system::ProcessId &value) const noexcept -> std::size_t { return value.toHash(); }
};
