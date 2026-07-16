// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharRange.hpp"

#include <array>
#include <memory>
#include <vector>

namespace erbsland::re::impl {

/// A combined character class that consists of multiple character ranges.
class CharClass {
    using RangeVector = std::vector<CharRange>;
    using RangesPtr = std::shared_ptr<RangeVector>;
    struct Data {
        RangesPtr ranges;         ///< The shared vector of ranges.
        std::size_t hash = 0;     ///< The hash of this object.
        bool readyForUse = false; ///< Flag if this object is ready to use.
    };
    using DataPtr = std::shared_ptr<Data>;

public:
    /// Create a new char ranges instance from a vector of character ranges.
    /// @param ranges The character ranges to combine.
    template <typename Fwd>
        requires std::constructible_from<std::vector<CharRange>, Fwd>
    explicit CharClass(Fwd &&ranges) :
        _data{std::make_shared<Data>(std::make_shared<RangeVector>(std::forward<Fwd>(ranges)))} {}
    CharClass() : _data{std::make_shared<Data>(std::make_shared<RangeVector>(std::vector<CharRange>()))} {}

    // defaults
    ~CharClass() = default;
    CharClass(const CharClass &) noexcept = default;
    CharClass(CharClass &&) noexcept = default;
    auto operator=(const CharClass &) noexcept -> CharClass & = default;
    auto operator=(CharClass &&) noexcept -> CharClass & = default;
    auto operator==(const CharClass &) const -> bool;
    auto operator!=(const CharClass &) const -> bool;

    /// Create and prepare a hash for use.
    template <typename Fwd>
        requires std::constructible_from<std::vector<CharRange>, Fwd>
    static auto createPrepared(Fwd &&ranges) -> CharClass {
        auto result = CharClass(std::forward<Fwd>(ranges));
        result.prepareForUse();
        return result;
    }

public: // build the ranges
    /// Add a single character to this range.
    void add(text::Char character);
    /// Add a character range.
    /// @param first The first character of the range.
    /// @param last The last character of the range.
    void add(text::Char first, text::Char last);

public: // Accessors
    /// Access the current size.
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    /// Test if this is a single character.
    [[nodiscard]] auto isSingleChar() const noexcept -> bool;

public: // normalize, prepare for use.
    /// Repare this ranges object to be used.
    void prepareForUse();

public: // use
    /// Test if the given character matches this range.
    [[nodiscard]] auto matches(text::Char character) const -> bool;
    /// Create a string representation of the ranges.
    /// This basically joins all character ranges.
    [[nodiscard]] auto toString() const -> text::String;
    /// Access the hash.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return _data->hash; }
    /// Access the ranges.
    [[nodiscard]] auto ranges() const noexcept -> const RangeVector & { return *_data->ranges; }

private:
    /// Normalize a vector of ranges: sort by first, then merge overlapping or adjacent ranges.
    /// After this, the vector is strictly ordered and non-overlapping.
    void normalizeRanges() noexcept;
    /// Build the hash for this object.
    void buildHash() noexcept;
    /// Create a new copy of the ranges vector if necessary.
    void conditionalDetach();

private:
    DataPtr _data;
};

}

namespace std {
template <>
struct hash<erbsland::re::impl::CharClass> {
    auto operator()(const erbsland::re::impl::CharClass &charClass) const noexcept -> std::size_t {
        return charClass.hash();
    }
};
}
