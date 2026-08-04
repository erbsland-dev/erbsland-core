// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Flag.hpp"

#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../util/EnumFlags.hpp"

#include <array>

namespace erbsland::re {

/// Flags for constructing a regular expression object.
/// @tested{FeaturesAndSettingsTest}
class Flags : public util::EnumFlags<Flag, Flags> {
    using Base = util::EnumFlags<Flag, Flags>;

public:
    using Base::Base;

public: // diagnostics
    /// Create a diagnostic string for the flags.
    [[nodiscard]] auto toString() const -> text::String {
        using namespace text::literals;
        text::StringEditor result;
        for (const auto flag : all()) {
            if (!isSet(flag)) {
                continue;
            }
            if (!result.isEmpty()) {
                result.append(", "_el);
            }
            result.append(re::toString(flag));
        }
        return result;
    }

public: // helpers
    /// Get every supported regular-expression flag in stable order.
    [[nodiscard]] constexpr static auto all() noexcept -> std::array<Flag, 6> {
        return std::array<Flag, 6>{
            Flag::IgnoreCase, Flag::Multiline, Flag::DotAll, Flag::Ascii, Flag::Verbose, Flag::CRLF};
    }
};

}
