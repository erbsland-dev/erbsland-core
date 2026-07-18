// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../engine/CaptureGroupNames.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../CaptureGroup.hpp"
#include "../../Match.hpp"

#include <variant>

namespace erbsland::re::impl {

/// A replacement text for regular-expression matches.
class Replacement {
public:
    /// Static text that is inserted as-it-is.
    struct StaticText {
        text::String text;
    };
    /// A reference to a capture group.
    struct CaptureGroup {
        CaptureGroupIndex index;
    };
    using Part = std::variant<StaticText, CaptureGroup>;

public:
    Replacement() = default;
    Replacement(const Replacement &) = default;
    Replacement(Replacement &&) = default;
    Replacement &operator=(const Replacement &) = default;
    Replacement &operator=(Replacement &&) = default;
    ~Replacement() = default;

public: // setup
    /// Add a new part with static text.
    void addStaticText(const text::String &text) { _parts.emplace_back(StaticText{text}); }
    /// Add a new part with a capture group.
    void addCaptureGroup(const CaptureGroupIndex index) { _parts.emplace_back(CaptureGroup{index}); }
    /// Add a new part.
    void addPart(Part part) { _parts.emplace_back(std::move(part)); }

public: // use
    /// Append this replacement to the given text.
    /// @param text The text to edit.
    /// @param match The match to use for the replacement.
    void appendTo(text::StringEditor &text, const MatchPtr &match) const;

    /// Get the size of the replacement for the given match.
    /// @param match The match to use for the replacement.
    [[nodiscard]] auto length(const MatchPtr &match) const -> unit::ByteLength;

public:
    /// Create a replacement for the given format string.
    ///
    /// Use `{n}` to refer to captured group contents in the replacement string, where `n` stands for a decimal
    /// number referring to the captured group. Use `{0}` to refer to the entire match.
    /// Use `{name}` to refer to captured group contents by name in the replacement string.
    /// To use `{` or `}` in the replacement text, escape them by doubling them `{{` and `}}`.
    ///
    /// @param expression The format string to parse.
    /// @param groupNames The names of the capture groups.
    /// @return The replacement object.
    /// @throws RegExError in case of an invalid format string.
    [[nodiscard]] static auto create(const text::String &expression, const CaptureGroupNames &groupNames)
        -> Replacement;

private:
    std::vector<Part> _parts; ///< The parts
};

}
