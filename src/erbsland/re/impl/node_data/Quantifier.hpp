// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NodeData.hpp"

#include "../engine/CaptureGroupTypes.hpp"
#include "../error/InternalError.hpp"

#include <cstdint>
#include <limits>

namespace erbsland::re::impl::node_data {

/// A quantifier for repeating a single child node.
class Quantifier : public NodeData {
public:
    using CounterIndex = uint8_t;
    using Count = uint16_t;
    /// Select how repetitions are consumed by the regex engine.
    enum class Mode : uint8_t {
        Greedy,
        Lazy,
        Possessive,
    };

public:
    /// Fast constructor.
    /// @param content The single child to be repeated (may be null initially).
    /// @param counterIndex The index of the counter to use in the code.
    /// @param minimum The minimum number of repetitions.
    /// @param maximum The maximum number of repetitions.
    /// @param mode The quantifier mode.
    Quantifier(
        PatternNodePtr content,
        const CounterIndex counterIndex,
        const Count minimum,
        const Count maximum,
        const Mode mode) :
        content{std::move(content)}, counterIndex{counterIndex}, minimum{minimum}, maximum{maximum}, mode{mode} {

        ERBSLAND_CORE_RE_REQUIRE_SAFETY(this->content != nullptr, "Quantifier must have a child"_el);
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(this->minimum <= this->maximum, "Minimum cannot be greater than maximum"_el);
    }

    /// Default constructor for aggregate-style initialization and fast zero-init.
    constexpr Quantifier() = default;

public:
    /// Create a stable string used for validating node trees in tests.
    [[nodiscard]] auto toTestString() const -> text::String {
        auto result = text::StringEditor{text::StringFormat{"Quantifier(min={},max={},mode={}"}.build(
            strForCount(minimum), strForCount(maximum), nameForMode(mode))};
        if (atomicGroupId != cNoAtomicGroupId) {
            result.append(text::StringFormat{",atomicGroupId={}"}.build(atomicGroupId));
        }
        result.append(U')');
        return result;
    }

    /// Access the sole repeated child node.
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> { return {&content, 1U}; }

    /// Access the size of this data block.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return 1U; }

public:
    /// Return the sentinel representing an unbounded repetition count.
    constexpr static auto infinitelyMany() noexcept -> Count { return std::numeric_limits<Count>::max(); }
    /// Format a repetition count for diagnostic output.
    static auto strForCount(const Count count) -> text::String {
        using namespace text::literals;
        return count == infinitelyMany() ? "*"_el : text::String::fromInteger(count);
    }
    /// Format a quantifier mode for diagnostic output.
    static auto nameForMode(const Mode mode) -> text::String {
        using namespace text::literals;
        switch (mode) {
        case Mode::Greedy:
            return "greedy"_el;
        case Mode::Lazy:
            return "lazy"_el;
        case Mode::Possessive:
            return "possessive"_el;
        default:
            throwInternalError("Invalid quantifier mode"_el);
        }
    }

public:
    PatternNodePtr content;                        ///< The single child.
    CounterIndex counterIndex{0};                  ///< The counter-index to use.
    AtomicGroupId atomicGroupId{cNoAtomicGroupId}; ///< The unique ID of the atomic group.
    Count minimum{0};                              ///< Minimum number of repetitions.
    Count maximum{0};                              ///< Maximum number of repetitions.
    Mode mode{Mode::Greedy};                       ///< Greedy vs lazy.
};

}
