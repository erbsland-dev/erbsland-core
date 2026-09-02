// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::system {

/// A supported native CPU architecture.
/// @tested{SystemInfoTest}
class CpuArchitecture final {
public:
    /// CPU-architecture values.
    enum Value : std::uint8_t {
        Unknown, ///< An unknown or unsupported architecture.
        X86,     ///< 32-bit x86.
        X86_64,  ///< 64-bit x86.
        Arm32,   ///< 32-bit Arm.
        Arm64,   ///< 64-bit Arm.
    };

public:
    /// Create an architecture from its raw value.
    constexpr CpuArchitecture(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr CpuArchitecture() noexcept = default;
    ~CpuArchitecture() = default;
    CpuArchitecture(const CpuArchitecture &) = default;
    CpuArchitecture(CpuArchitecture &&) = default;
    auto operator=(const CpuArchitecture &) -> CpuArchitecture & = default;
    auto operator=(CpuArchitecture &&) -> CpuArchitecture & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const CpuArchitecture &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const CpuArchitecture &other, value, other._value);

public: // accessors
    /// Get the raw architecture value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Get the stable lowercase architecture identifier.
    [[nodiscard]] auto toString() const -> text::String;

private:
    Value _value{Unknown}; ///< Native CPU architecture value.
};

}
