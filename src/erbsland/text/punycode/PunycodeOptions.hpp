// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PunycodeMode.hpp"

#include "../CharSet.hpp"

#include <optional>
#include <utility>

namespace erbsland::text::punycode {

/// Options for Punycode and strict IDNA2008 processing.
/// @seedoc{/reference/text/encoding}
/// @tested{PunycodeTest IdnaTest}
class PunycodeOptions final {
public:
    /// Create pure RFC 3492 options.
    PunycodeOptions() = default;

public: // accessors
    /// Get the processing mode.
    [[nodiscard]] auto mode() const noexcept -> PunycodeMode { return _mode; }
    /// Set the processing mode.
    auto setMode(const PunycodeMode mode) noexcept -> PunycodeOptions & {
        _mode = mode;
        return *this;
    }
    /// Test whether an additional allowed-character filter is configured.
    [[nodiscard]] auto hasAllowedCharacters() const noexcept -> bool { return _allowedCharacters.has_value(); }
    /// Get the additional allowed-character filter.
    [[nodiscard]] auto allowedCharacters() const noexcept -> const std::optional<CharSet> & {
        return _allowedCharacters;
    }
    /// Set the additional allowed-character filter.
    auto setAllowedCharacters(CharSet allowedCharacters) -> PunycodeOptions & {
        _allowedCharacters = std::move(allowedCharacters);
        return *this;
    }
    /// Remove the additional allowed-character filter.
    auto clearAllowedCharacters() noexcept -> PunycodeOptions & {
        _allowedCharacters.reset();
        return *this;
    }

public: // factories
    /// Create pure RFC 3492 options.
    [[nodiscard]] static auto defaultOptions() noexcept -> PunycodeOptions { return {}; }
    /// Create strict IDNA2008 single-label options.
    [[nodiscard]] static auto idna2008Label() noexcept -> PunycodeOptions {
        return PunycodeOptions{}.setMode(PunycodeMode::Idna2008Label);
    }
    /// Create strict IDNA2008 domain-name options.
    [[nodiscard]] static auto idna2008Domain() noexcept -> PunycodeOptions {
        return PunycodeOptions{}.setMode(PunycodeMode::Idna2008Domain);
    }
    /// Create the strict options used at network boundaries.
    [[nodiscard]] static auto network() noexcept -> PunycodeOptions { return idna2008Domain(); }

private:
    PunycodeMode _mode{PunycodeMode::Pure};    ///< The processing mode.
    std::optional<CharSet> _allowedCharacters; ///< The optional additional Unicode filter.
};

}
