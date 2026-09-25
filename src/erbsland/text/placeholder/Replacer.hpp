// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Filter.hpp"
#include "Replacer_fwd.hpp"
#include "ReplacerOptions.hpp"
#include "Source.hpp"

#include "impl/Registry.hpp"

#include "../CharSet.hpp"
#include "../StringCharReader.hpp"
#include "../StringMap.hpp"

#include "../../unit/CpLength.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace erbsland::text::placeholder {

/// Replaces configured placeholders in ordinary UTF-8 strings.
/// Configure it once and reuse it with one instance per concurrently executing thread.
/// @seedoc{/topics/text_placeholders/placeholders}
/// @tested{ReplacerTest}
class Replacer final {
    /// One parsed source or filter request.
    struct Part final {
        String name;      ///< Normalized provider name.
        String parameter; ///< Case-preserving parameter text.
    };
    /// The syntax token found at the reader position.
    enum class Delimiter : uint8_t {
        None,   ///< Ordinary content.
        End,    ///< Closing frame.
        Filter, ///< Filter separator.
        Name,   ///< Name/parameter separator.
    };

public:
    /// Create a local replacer with validated syntax options.
    explicit Replacer(ReplacerOptions options = {});
    /// Create a shared replacer with validated syntax options.
    [[nodiscard]] static auto create(ReplacerOptions options = {}) -> ReplacerPtr;

public: // providers
    /// Add a source provider, rejecting null, invalid, or duplicate names.
    void addSource(const SourcePtr &source);
    /// Remove all names of a source provider by pointer identity.
    void removeSource(const SourcePtr &source) noexcept;
    /// Find a source by its case-insensitive name, or return null.
    [[nodiscard]] auto source(const String &name) const -> SourcePtr;
    /// Add a filter provider, rejecting null, invalid, or duplicate names.
    void addFilter(const FilterPtr &filter);
    /// Remove all names of a filter provider by pointer identity.
    void removeFilter(const FilterPtr &filter) noexcept;
    /// Find a filter by its case-insensitive name, or return null.
    [[nodiscard]] auto filter(const String &name) const -> FilterPtr;
    /// Register the built-in environment source under `env` or a custom name.
    void addEnvironmentSource(const String &name = {});
    /// Register or update the built-in variable source under `var` or a custom name.
    void setVariableSource(StringMap<String> variables, const String &name = {});
    /// Register all built-in text filters.
    void addTextFilters();

public: // operations
    /// Test expressions and provider parameters without producing replacement text.
    /// @throws err::LogicError If no source is registered.
    [[nodiscard]] auto validate(const String &input) const -> bool;
    /// Validate expressions and report the first error with a code-point offset.
    /// @throws err::LogicError If no source is registered.
    /// @throws ReplacerError For malformed syntax or rejected provider parameters.
    void validateOrThrow(const String &input) const;
    /// Replace valid expressions; keep invalid expressions unchanged and continue.
    /// @throws err::LogicError If no source is registered.
    [[nodiscard]] auto replace(const String &input) const -> String;
    /// Replace expressions or report the first error with a code-point offset.
    /// @throws err::LogicError If no source is registered.
    /// @throws ReplacerError For malformed syntax or provider failures.
    [[nodiscard]] auto replaceOrThrow(const String &input) const -> String;

private:
    /// Ensure the mandatory source registry is not empty.
    void requireSources() const;
    /// Scan the input in validation, tolerant replacement, or strict replacement mode.
    [[nodiscard]] auto process(const String &input, bool validation, bool tolerant) const -> String;
    /// Parse and optionally evaluate one expression after its opening frame.
    [[nodiscard]] auto parseExpression(StringCharReader &reader, bool validation, bool &closed) const -> String;
    /// Parse one source or filter part.
    [[nodiscard]] auto parsePart(StringCharReader &reader) const -> Part;
    /// Parse name or parameter content up to a delimiter.
    [[nodiscard]] auto parseContent(StringCharReader &reader, bool isName) const -> String;
    /// Pick the longest active delimiter, with end/filter/name priority on ties.
    [[nodiscard]] auto delimiterAt(const StringCharReader &reader, bool isName) const -> Delimiter;
    /// Check whether a sequence matches at the reader position.
    [[nodiscard]] static auto matches(const StringCharReader &reader, const String &sequence) -> bool;
    /// Consume an escaped opening frame in double mode.
    [[nodiscard]] auto advanceDoubledBegin(StringCharReader &reader) const -> bool;
    /// Skip the failed expression to its closing frame or line end.
    void recover(StringCharReader &reader) const;

private:
    ReplacerOptions _options;     ///< Immutable expression syntax.
    impl::Registry _registry;     ///< Registered source and filter providers.
    Char _beginFirst;             ///< First opening-frame code point.
    Char _endFirst;               ///< First closing-frame code point.
    Char _filterFirst;            ///< First filter-separator code point, or a signal.
    Char _nameFirst;              ///< First name-separator code point, or a signal.
    unit::CpLength _endLength;    ///< Closing-frame length.
    unit::CpLength _filterLength; ///< Filter-separator length.
    unit::CpLength _nameLength;   ///< Name-separator length.
    CharSet _stopSet;             ///< Characters that can start an expression or escape.
};

}
