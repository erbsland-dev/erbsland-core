// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ArgumentUnit.hpp"
#include "../unit/ByteIndex.hpp"

#include <vector>

namespace erbsland::options {

/// The location of sensitive text in one command-line argument.
/// Sensitive text extends from `startIndex()` through the end of the argument.
/// @tested{OptionsParserTest}
class OptionSensitiveTextLocation final {
public:
    /// Create an empty location.
    OptionSensitiveTextLocation() = default;
    /// Create a sensitive-text location.
    /// @param argumentIndex The command-line argument containing the sensitive suffix.
    /// @param startIndex The UTF-8 byte index where the sensitive suffix starts.
    OptionSensitiveTextLocation(unit::ArgumentIndex argumentIndex, unit::ByteIndex startIndex) noexcept :
        _argumentIndex{argumentIndex}, _startIndex{startIndex} {}

    // defaults
    ~OptionSensitiveTextLocation() = default;
    OptionSensitiveTextLocation(const OptionSensitiveTextLocation &) = default;
    OptionSensitiveTextLocation(OptionSensitiveTextLocation &&) noexcept = default;
    auto operator=(const OptionSensitiveTextLocation &) -> OptionSensitiveTextLocation & = default;
    auto operator=(OptionSensitiveTextLocation &&) noexcept -> OptionSensitiveTextLocation & = default;

public: // operators
    /// Compare two locations.
    [[nodiscard]] auto operator==(const OptionSensitiveTextLocation &) const noexcept -> bool = default;

public: // accessors
    /// Get the command-line argument index.
    /// @return The argument index, or `ArgumentIndex::noIndex()` for an empty location.
    [[nodiscard]] auto argumentIndex() const noexcept -> unit::ArgumentIndex { return _argumentIndex; }
    /// Get the UTF-8 byte index where sensitive text starts.
    /// @return The start byte index, or `ByteIndex::noIndex()` for an empty location.
    [[nodiscard]] auto startIndex() const noexcept -> unit::ByteIndex { return _startIndex; }

private:
    unit::ArgumentIndex _argumentIndex{unit::ArgumentIndex::noIndex()}; ///< The source argument index.
    unit::ByteIndex _startIndex{unit::ByteIndex::noIndex()};            ///< The sensitive suffix start.
};

/// A list of sensitive command-line text locations.
using OptionSensitiveTextLocations = std::vector<OptionSensitiveTextLocation>;

}
