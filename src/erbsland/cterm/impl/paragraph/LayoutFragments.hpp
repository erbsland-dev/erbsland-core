// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutFragment.hpp"

#include <vector>

namespace erbsland::cterm::impl::paragraph {

/// Owns the rendered fragments for one physical paragraph line.
class LayoutFragments final {
public:
    using Container = std::vector<LayoutFragment>;
    using const_iterator = Container::const_iterator;

public: // defaults/deletions
    LayoutFragments() = default;
    ~LayoutFragments() = default;
    LayoutFragments(const LayoutFragments &) = delete;
    LayoutFragments(LayoutFragments &&) noexcept = default;
    auto operator=(const LayoutFragments &) -> LayoutFragments & = delete;
    auto operator=(LayoutFragments &&) noexcept -> LayoutFragments & = default;

public:
    /// Test whether no fragments have been appended yet.
    /// @return `true` if no fragments have been appended yet.
    [[nodiscard]] auto empty() const noexcept -> bool { return _fragments.empty(); }
    /// Get the rendered width of all owned fragments.
    /// @return The rendered width of all owned fragments.
    [[nodiscard]] auto width() const noexcept -> int { return _width; }
    /// Append a literal marker fragment.
    /// @param type The literal fragment type to append.
    /// @param width The rendered display width of the literal fragment.
    void appendLiteral(const LayoutFragment::Type type, const int width) {
        if (width <= 0) {
            return;
        }
        _fragments.emplace_back(type, width);
        _width += width;
    }
    /// Append a source-text range fragment.
    /// Adjacent source ranges are coalesced when possible.
    /// @param startIndex The first source character index.
    /// @param length The number of source characters.
    /// @param width The rendered display width of the source range.
    void appendSourceRange(const BlockIndex startIndex, const BlockCount length, const int width) {
        if (length.isZero() || width <= 0) {
            return;
        }
        if (!_fragments.empty()) {
            auto &last = _fragments.back();
            if (last.type() == LayoutFragment::Type::SourceRange && last.startIndex() + last.length() == startIndex) {
                last.addLength(length);
                last.addWidth(width);
                _width += width;
                return;
            }
        }
        _fragments.emplace_back(startIndex, length, width);
        _width += width;
    }
    /// Append generated spaces.
    /// Adjacent spaces with the same color are coalesced.
    /// @param width The rendered width of the spaces.
    /// @param color The color applied to the generated spaces.
    void appendSpaces(const int width, const Color color) {
        if (width <= 0) {
            return;
        }
        if (!_fragments.empty()) {
            auto &last = _fragments.back();
            if (last.type() == LayoutFragment::Type::Spaces && last.color() == color) {
                last.addWidth(width);
                _width += width;
                return;
            }
        }
        _fragments.emplace_back(LayoutFragment::Type::Spaces, width, color);
        _width += width;
    }
    /// Get an iterator to the first fragment.
    /// @return An iterator to the first fragment.
    [[nodiscard]] auto begin() const noexcept -> const_iterator { return _fragments.begin(); }
    /// Get an iterator past the last fragment.
    /// @return An iterator past the last fragment.
    [[nodiscard]] auto end() const noexcept -> const_iterator { return _fragments.end(); }

private:
    Container _fragments;
    int _width = 0;
};

}
