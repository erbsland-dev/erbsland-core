// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaAttributes.hpp"
#include "IdnaData.hpp"

#include "../unicode/IdnaMapping.hpp"
#include "../unicode/NfcNormalizer.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/String.hpp>

#include <span>
#include <vector>

namespace tool::idna {

using namespace el::text::literals;

/// Prepare PSL domains using source-derived nontransitional mapping, NFC, and IDNA2008.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
class LabelProcessor final {
    /// Prefix identifying ASCII-compatible IDNA labels for Punycode decoding and encoding.
    static constexpr auto cALabelPrefix = "xn--"_el;

    /// A code point with source-derived validation attributes.
    struct CodePoint final {
        char32_t value;            ///< Decoded scalar.
        IdnaAttributes attributes; ///< Attributes from the selected source snapshot.
    };
    /// A prepared Unicode label.
    struct PreparedLabel final {
        el::String text;                   ///< NFC Unicode label.
        std::vector<CodePoint> codePoints; ///< Contiguous validation working storage.
    };

public:
    /// Bind the verified properties and preprocessing data.
    /// @param data IDNA validation attributes.
    /// @param normalizer Source-backed NFC processing.
    /// @param mapping Official UTS #46 mapping.
    LabelProcessor(const IdnaData &data, const unicode::NfcNormalizer &normalizer, const unicode::IdnaMapping &mapping);

public: // conversion
    /// Convert a rule base to canonical ASCII.
    /// @param text Unicode or ASCII domain, without wildcard/exception syntax.
    /// @return The canonical ASCII domain.
    /// @throws el::ApplicationError If source-derived mapping or normalization fails.
    /// @throws el::ParseError If Punycode conversion, IDNA validation, or DNS length checks fail.
    [[nodiscard]] auto toAscii(const el::String &text) const -> el::String;

private:
    /// Select A-label decoding or direct Unicode label validation.
    /// @param label One mapped, NFC label.
    /// @return The validated Unicode label and source-derived attributes.
    /// @throws el::ApplicationError If source-derived normalization fails.
    /// @throws el::ParseError If Punycode conversion or label validation fails.
    [[nodiscard]] auto prepareLabel(const el::String &label) const -> PreparedLabel;
    /// Decode and validate an A-label with a canonical Punycode round trip.
    /// @param label The ASCII label beginning with xn--.
    /// @return The validated NFC Unicode label and attributes.
    /// @throws el::ApplicationError If source-derived normalization fails.
    /// @throws el::ParseError If syntax, Punycode conversion, validation, or the round trip fails.
    [[nodiscard]] auto decodeALabel(const el::String &label) const -> PreparedLabel;
    /// Apply strict IDNA2008 syntax, status, and contextual checks.
    /// @param label The NFC Unicode label.
    /// @return The validated label with its source-derived attributes.
    /// @throws el::ParseError If a label rule fails.
    [[nodiscard]] auto validateUnicodeLabel(const el::String &label) const -> PreparedLabel;
    /// Decode a label and attach source-derived validation attributes.
    /// @param label The Unicode label to inspect.
    /// @return Decoded scalars with their IDNA attributes.
    [[nodiscard]] auto codePoints(const el::String &label) const -> std::vector<CodePoint>;
    /// Encode a validated label and verify its Unicode round trip.
    /// @param label The prepared Unicode label.
    /// @return An ASCII label, using Punycode when necessary.
    /// @throws el::ParseError If Punycode conversion, DNS length checks, or verification fails.
    [[nodiscard]] static auto encodeLabel(const PreparedLabel &label) -> el::String;
    /// Check a contextual character using its surrounding label attributes.
    /// @param values The complete label's scalars and source attributes.
    /// @param index The contextual character's position.
    /// @param status The contextual IDNA status to check.
    /// @throws el::ParseError If the contextual rule fails.
    static void validateContext(std::span<const CodePoint> values, std::size_t index, IdnaStatus status);
    /// Check the Virama and joining-type rules for a CONTEXTJ character.
    /// @param values The complete label's scalars and source attributes.
    /// @param index The contextual character's position.
    static void validateJoiningContext(std::span<const CodePoint> values, std::size_t index);
    /// Check the script, punctuation, and digit rules for a CONTEXTO character.
    /// @param values The complete label's scalars and source attributes.
    /// @param index The contextual character's position.
    static void validateOtherContext(std::span<const CodePoint> values, std::size_t index);
    /// Test whether a direction is allowed in a label of the selected direction.
    /// @param direction The character's direction.
    /// @param rtl Whether the label is right-to-left.
    /// @return Whether the direction is allowed.
    [[nodiscard]] static auto isAllowedBidi(IdnaBidi direction, bool rtl) -> bool;
    /// Test whether a non-NSM direction may end the selected label.
    /// @param direction The character's direction.
    /// @param rtl Whether the label is right-to-left.
    /// @return Whether this is a valid ending.
    [[nodiscard]] static auto isBidiEnding(IdnaBidi direction, bool rtl) -> bool;
    /// Apply IDNA bidi rules to a prepared label.
    /// @param values The complete label's scalars and source attributes.
    /// @param force Whether to apply bidi checks even without an RTL character.
    /// @throws el::ParseError If a bidi rule fails.
    static void validateBidi(std::span<const CodePoint> values, bool force);
    /// Validate allowed directions, endings, and numeral consistency for a bidi label.
    /// @param values The complete label's scalars and source attributes.
    /// @param rtl Whether the label is right-to-left.
    static void validateBidiSequence(std::span<const CodePoint> values, bool rtl);
    /// Enforce nonempty ASCII label or domain DNS length limits.
    /// @param text The encoded ASCII text.
    /// @param label Whether to enforce the single-label rather than domain limit.
    /// @throws el::ParseError If the text violates DNS length limits.
    static void validateAsciiLength(const el::String &text, bool label);

private:
    const IdnaData &_data;                     ///< Source-derived IDNA attributes.
    const unicode::NfcNormalizer &_normalizer; ///< Source-backed canonical normalization.
    const unicode::IdnaMapping &_mapping;      ///< Source-backed compatibility mapping.
};

}
