// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Feature.hpp"

#include "../text/Literals.hpp"
#include "../text/StdFormatForText.hpp"
#include "../text/String.hpp"
#include "../util/EnumFlags.hpp"

#include <array>
#include <format>

namespace erbsland::re {

/// Features for compiling regular expressions.
/// @tested{FeaturesAndSettingsTest}
class Features : public util::EnumFlags<Feature, Features> {
    using Base = util::EnumFlags<Feature, Features>;

public:
    using Base::Base;

public: // diagnostics
    /// Create a diagnostic string for the enabled features.
    [[nodiscard]] auto toString() const -> text::String {
        using namespace text::literals;
        text::String result;
        for (const auto feature : all()) {
            if (!isSet(feature)) {
                continue;
            }
            if (!result.isEmpty()) {
                result.append(", "_el);
            }
            result.append(re::toString(feature));
        }
        return result;
    }

public: // helpers
    [[nodiscard]] constexpr static auto all() noexcept -> std::array<Feature, 16> {
        return std::array<Feature, 16>{
            Feature::QuotedLiterals,
            Feature::EscapeBell,
            Feature::EscapeControl,
            Feature::EscapeEscape,
            Feature::EscapeFormFeed,
            Feature::EscapeOctal,
            Feature::EscapeHex,
            Feature::EscapeLongUnicode,
            Feature::EscapeHorizontalSpace,
            Feature::EscapeVerticalSpace,
            Feature::PosixClasses,
            Feature::AnchorLowercaseZ,
            Feature::EmptyAlternatives,
            Feature::EmptyGroups,
            Feature::AcceptNullInPattern,
            Feature::AcceptNullInInput,
        };
    }
};

}

template <>
struct std::formatter<erbsland::re::Features> : std::formatter<erbsland::text::String> {
    auto format(const erbsland::re::Features features, std::format_context &ctx) const {
        return std::formatter<erbsland::text::String>::format(features.toString(), ctx);
    }
};
