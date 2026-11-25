// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

#include <array>
#include <compare>
#include <cstdint>
#include <optional>

namespace tool::github_workflows {

/// An exact stable tag and its numerically comparable version components.
class StableTag final {
public: // conversion
    /// Recognize exactly vMAJOR.MINOR.PATCH using checked unsigned decimal components.
    /// @param text Complete tag spelling; prereleases and abbreviated tags are excluded.
    /// @return A parsed tag, or no value for an invalid or overflowing component.
    [[nodiscard]] static auto fromString(const el::String &text) -> std::optional<StableTag>;
    /// Get the original tag spelling.
    [[nodiscard]] auto toString() const -> const el::String & { return _text; }

public: // operators
    auto operator<=>(const StableTag &other) const -> std::strong_ordering { return _parts <=> other._parts; }
    auto operator==(const StableTag &other) const -> bool { return _parts == other._parts; }

private:
    el::String _text;                 ///< Original tag spelling for exact remote lookup and annotation.
    std::array<uint64_t, 3> _parts{}; ///< Numeric major, minor, and patch components.
};

}
