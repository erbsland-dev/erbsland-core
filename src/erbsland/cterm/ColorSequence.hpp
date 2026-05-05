// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Color.hpp"

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace erbsland::cterm {

/// A configurable sequence of complete `Color` values with run-length style counts.
///
/// Each effective position in the sequence yields one full foreground/background color pair.
/// The sequence can be accessed by cyclic index or by a normalized value in the range `0.0..1.0`.
/// Consumers may perform multiple lookups, for example to combine the foreground of one entry with the background of
/// a neighboring entry.
/// @tested `ColorSequenceTest`
class ColorSequence final {
public:
    /// One run-length encoded `Color` entry in the sequence.
    struct Entry final {
        /// The color value for this entry.
        Color color{};
        /// How many effective positions this color occupies.
        std::size_t count{1};
    };

public:
    /// Create an empty color sequence.
    ColorSequence() = default;
    /// Create a sequence with one repeated full-color entry.
    /// @param color The color to add.
    /// @param count How often the color should repeat.
    explicit ColorSequence(Color color, std::size_t count = 1) noexcept;
    /// Create a sequence from individual full-color entries with count `1`.
    /// @param colors The colors to add in order.
    ColorSequence(std::initializer_list<Color> colors) noexcept;
    /// Create a sequence from explicit run-length encoded entries.
    /// @param entries The entries to add in order.
    ColorSequence(std::initializer_list<Entry> entries) noexcept;

    // defaults
    ~ColorSequence() = default;
    ColorSequence(const ColorSequence &) noexcept = default;
    ColorSequence(ColorSequence &&) noexcept = default;
    auto operator=(const ColorSequence &) noexcept -> ColorSequence & = default;
    auto operator=(ColorSequence &&) noexcept -> ColorSequence & = default;

public: // modifiers
    /// Add one full-color entry to this sequence.
    /// @param color The color for this entry.
    /// @param count How often this color should repeat in the effective sequence.
    void add(Color color, std::size_t count = 1) noexcept;

public: // accessors
    /// Get one complete color entry by effective sequence index.
    /// Indexes wrap around to support cyclic access.
    /// @param index The effective sequence index.
    /// @return The full color at the wrapped index, or the inherited color if the sequence is empty.
    [[nodiscard]] auto color(std::size_t index) const noexcept -> Color;
    /// Get a color by normalized position in the range `0.0..1.0`.
    /// Values outside this range are clamped.
    /// @param normalized The normalized sequence position.
    /// @return The color at the requested normalized position.
    [[nodiscard]] auto colorNormalized(double normalized) const noexcept -> Color;
    /// Effective sequence length (sum of all counts).
    [[nodiscard]] auto sequenceLength() const noexcept -> std::size_t { return _sequenceLength; }
    /// Number of configured entries.
    [[nodiscard]] auto entryCount() const noexcept -> std::size_t { return _entries.size(); }

public: // tests
    /// Test if this sequence has no effective entries.
    [[nodiscard]] auto empty() const noexcept -> bool { return _sequenceLength == 0; }

private:
    [[nodiscard]] auto colorAtEffectiveIndex(std::size_t index) const noexcept -> Color;

private:
    std::vector<Entry> _entries;
    std::size_t _sequenceLength{0};
};

}
