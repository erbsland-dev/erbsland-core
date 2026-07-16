// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Features.hpp"

#include "impl/Limits.hpp"

#include "../text/StdFormatForText.hpp"
#include "../text/StringFormat.hpp"
#include "../unit/CpLength.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>

namespace erbsland::re {

/// Settings for compiling regular expression patterns.
class Settings {
public:
    // defaults
    Settings() = default;
    ~Settings() = default;
    auto operator=(const Settings &) -> Settings & = default;
    auto operator=(Settings &&) -> Settings & = default;
    Settings(const Settings &) = default;
    Settings(Settings &&) = default;

public: // limits
    /// The maximum pattern length in characters.
    [[nodiscard]] auto maximumPatternLength() const noexcept -> unit::CpLength {
        return std::min(_maximumPatternLength, impl::limits::maximumPatternLength);
    }

    /// Set the maximum pattern length.
    /// @param length A length in characters, or zero to reset back to default.
    void setMaximumPatternLength(unit::CpLength length) noexcept {
        if (length.isZero()) {
            length = impl::limits::maximumPatternLength;
        }
        _maximumPatternLength = std::min(length, impl::limits::maximumPatternLength);
    }

    /// The maximum group nesting depth.
    [[nodiscard]] auto maximumGroupNestingDepth() const noexcept -> std::size_t {
        return std::min(_maximumGroupNestingDepth, impl::limits::maximumGroupNestingDepth);
    }

    /// Set the maximum group nesting depth.
    /// @param depth A depth in levels, or zero to reset back to default.
    void setMaximumGroupNestingDepth(std::size_t depth) noexcept {
        if (depth == 0) {
            depth = impl::limits::maximumGroupNestingDepth;
        }
        _maximumGroupNestingDepth = std::min(depth, impl::limits::maximumGroupNestingDepth);
    }

    /// The maximum capture group count.
    [[nodiscard]] auto maximumCaptureGroupCount() const noexcept -> std::size_t {
        return std::min(_maximumCaptureGroupCount, impl::limits::maximumCaptureGroupCount);
    }

    /// Set the maximum capture group count.
    /// @param count The maximum capture group count or zero to reset back to default.
    void setMaximumCaptureGroupCount(std::size_t count) noexcept {
        if (count == 0) {
            count = impl::limits::maximumCaptureGroupCount;
        }
        _maximumCaptureGroupCount = std::min(count, impl::limits::maximumCaptureGroupCount);
    }

    /// The maximum sequence length
    [[nodiscard]] auto maximumSequenceLength() const noexcept -> std::size_t {
        return std::min(_maximumSequenceLength, impl::limits::maximumSequenceLength);
    }

    /// Set the maximum sequence length.
    /// @param length A length in characters, or zero to reset back to default.
    void setMaximumSequenceLength(std::size_t length) noexcept {
        if (length == 0) {
            length = impl::limits::maximumSequenceLength;
        }
        _maximumSequenceLength = std::min(length, impl::limits::maximumSequenceLength);
    }

    /// The maximum alternative count
    [[nodiscard]] auto maximumAlternativeCount() const noexcept -> std::size_t {
        return std::min(_maximumAlternativeCount, impl::limits::maximumAlternativeCount);
    }

    /// Set the maximum alternative count.
    /// @param count A count of alternatives, or zero to reset back to default.
    void setMaximumAlternativeCount(std::size_t count) noexcept {
        if (count == 0) {
            count = impl::limits::maximumAlternativeCount;
        }
        _maximumAlternativeCount = std::min(count, impl::limits::maximumAlternativeCount);
    }

    /// Get the maximum quantifier count.
    /// This is the maximum for quantifier expressions like `{n,m}`
    [[nodiscard]] auto maximumQuantifierCount() const noexcept -> std::size_t {
        return std::min(_maximumQuantifierCount, impl::limits::maximumQuantifierCount);
    }

    /// Set the maximum quantifier count.
    /// @param count The maximum quantifier count, or zero to reset back to default.
    void setMaximumQuantifierCount(std::size_t count) noexcept {
        if (count == 0) {
            count = impl::limits::maximumQuantifierCount;
        }
        _maximumQuantifierCount = std::min(count, impl::limits::maximumQuantifierCount);
    }

public: // timeout
    /// Access the current timeout.
    /// @return The current timeout. Zero means no timeout.
    [[nodiscard]] auto timeout() const noexcept -> std::chrono::milliseconds { return _timeout; }

    /// Set a timeout for all calls.
    /// @param timeout The timeout to set, or zero to disable timeout.
    void setTimeout(const std::chrono::milliseconds timeout) noexcept { _timeout = timeout; }

public: // features
    /// Test if a feature is enabled.
    /// @param feature The feature to test.
    [[nodiscard]] auto hasFeature(const Feature feature) const noexcept -> bool { return _features.isSet(feature); }

    /// Disable a feature.
    /// @param feature The feature to disable.
    void disableFeature(const Feature feature) noexcept { _features.clear(feature); }

    /// Enable a feature
    /// @param feature The feature to enable.
    void enableFeature(const Feature feature) noexcept { _features.set(feature); }

public: // diagnostics
    /// Create a string representation for this settings object.
    [[nodiscard]] auto toString() const -> text::String {
        return text::StringFormat{
            "Settings(patternLength={}, groupDepth={}, captureGroupCount={}, sequenceLength={}, alternativeCount={}, "
            "quantifierCount={}, timeout={} ms, features=[{}])"}
            .build(
                _maximumPatternLength,
                _maximumGroupNestingDepth,
                _maximumCaptureGroupCount,
                _maximumSequenceLength,
                _maximumAlternativeCount,
                _maximumQuantifierCount,
                _timeout.count(),
                _features.toString());
    }

private:
    unit::CpLength _maximumPatternLength = impl::limits::maximumPatternLength;
    std::size_t _maximumGroupNestingDepth = impl::limits::maximumGroupNestingDepth;
    std::size_t _maximumCaptureGroupCount = impl::limits::maximumCaptureGroupCount;
    std::size_t _maximumSequenceLength = impl::limits::maximumSequenceLength;
    std::size_t _maximumAlternativeCount = impl::limits::maximumAlternativeCount;
    std::size_t _maximumQuantifierCount = impl::limits::maximumQuantifierCount;
    std::chrono::milliseconds _timeout = std::chrono::milliseconds::zero();
    Features _features = Features{Feature::Default};
};

}

template <>
struct std::formatter<erbsland::re::Settings> : std::formatter<erbsland::text::String> {
    auto format(const erbsland::re::Settings &settings, std::format_context &ctx) const {
        return std::formatter<erbsland::text::String>::format(settings.toString(), ctx);
    }
};
