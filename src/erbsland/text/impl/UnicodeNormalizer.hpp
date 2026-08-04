// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../NormalizationForm.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Normalize one source-aligned window using fixed stack storage.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
class UnicodeNormalizer final {
private:
    static constexpr auto cMaximumDecompositionLength = std::size_t{18};
    static constexpr auto cMaximumNonStarterCount = std::size_t{30};
    static constexpr auto cMaximumSequenceLength = cMaximumNonStarterCount + 1U;
    static constexpr auto cMaximumWindowLength = std::size_t{64};

    static constexpr auto cHangulSyllableBase = char32_t{0xAC00};
    static constexpr auto cHangulLeadingBase = char32_t{0x1100};
    static constexpr auto cHangulVowelBase = char32_t{0x1161};
    static constexpr auto cHangulTrailingBase = char32_t{0x11A7};
    static constexpr auto cHangulLeadingCount = 19U;
    static constexpr auto cHangulVowelCount = 21U;
    static constexpr auto cHangulTrailingCount = 28U;
    static constexpr auto cHangulBlockCount = cHangulVowelCount * cHangulTrailingCount;
    static constexpr auto cHangulSyllableCount = cHangulLeadingCount * cHangulBlockCount;

public:
    /// Create a normalizer for one explicit normalization form.
    explicit UnicodeNormalizer(NormalizationForm form) noexcept;

public:
    /// Prepare one decoded source scalar and test whether it starts a new source-aligned window.
    /// Call `appendPrepared()` after optionally finishing and resetting the previous window.
    /// @param codePoint A valid decoded Unicode scalar value.
    /// @return `true` if the current window is complete before this source scalar.
    [[nodiscard]] auto prepare(char32_t codePoint) -> bool;
    /// Append the source scalar prepared by the preceding call to `prepare()`.
    void appendPrepared() noexcept;
    /// Complete the current window, including its active canonical sequence.
    void finish() noexcept;
    /// Reset the completed window while retaining the scalar prepared for the next one.
    void resetWindow() noexcept;

public: // accessors
    /// Test if the current window has no source scalars.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _sourceLength == 0U; }
    /// Test if the completed normalized scalars equal the decoded source scalars.
    [[nodiscard]] auto isUnchanged() const noexcept -> bool;
    /// Access the completed normalized scalar sequence.
    [[nodiscard]] auto normalizedCodePoints() const noexcept -> std::span<const char32_t>;

private:
    /// Test if the selected form applies compatibility decompositions.
    [[nodiscard]] auto usesCompatibilityDecomposition() const noexcept -> bool;
    /// Test if the selected form applies canonical composition.
    [[nodiscard]] auto usesCanonicalComposition() const noexcept -> bool;
    /// Recursively append the selected decomposition into the prepared-scalar buffer.
    void appendDecomposition(char32_t codePoint) noexcept;
    /// Append one fully decomposed scalar to the active canonical sequence.
    void appendDecomposed(char32_t codePoint) noexcept;
    /// Complete the active canonical sequence into the normalized window.
    void finishSequence() noexcept;
    /// Append one scalar to the normalized window.
    void appendNormalized(char32_t codePoint) noexcept;
    /// Find a canonical composition for a pair, including algorithmic Hangul composition.
    [[nodiscard]] static auto compositionFor(char32_t starter, char32_t trailing) noexcept -> char32_t;
    /// Apply algorithmic Hangul composition for a pair.
    [[nodiscard]] static auto composeHangul(char32_t starter, char32_t trailing) noexcept -> char32_t;

private:
    NormalizationForm _form;                                            ///< The selected normalization form.
    std::array<char32_t, cMaximumDecompositionLength> _decomposition{}; ///< Prepared recursive decomposition.
    std::size_t _decompositionLength{};                                 ///< Used prepared decomposition entries.
    char32_t _preparedSource{};                               ///< Source scalar for the prepared decomposition.
    bool _isPrepared{};                                       ///< Whether a source scalar is prepared.
    std::array<char32_t, cMaximumWindowLength> _source{};     ///< Decoded source scalars in this window.
    std::size_t _sourceLength{};                              ///< Used source-scalar entries.
    std::array<char32_t, cMaximumWindowLength> _normalized{}; ///< Completed normalized scalars.
    std::size_t _normalizedLength{};                          ///< Used normalized entries.
    std::array<char32_t, cMaximumSequenceLength> _sequence{}; ///< Ordered active canonical sequence.
    std::size_t _sequenceLength{};                            ///< Used active-sequence entries.
    std::size_t _nonStarterCount{};                           ///< Non-starters in the active sequence.
    bool _sequenceOverflow{};                                 ///< Whether the active sequence exceeded the limit.
    bool _isFinished{};                                       ///< Whether the current window was completed.
};

}
