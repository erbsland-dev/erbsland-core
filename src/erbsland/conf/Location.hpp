// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceIdentifier.hpp"

#include "impl/utilities/InternalView.hpp"

#include "../unit/CodeLocation.hpp"

namespace erbsland::conf {

/// Represents the location in a parsed document.
/// @tested{LocationTest ValueLocationTest}
class Location {
public:
    /// Creates an undefined location.
    /// Can be tested with `isUndefined()`.
    Location() = default;
    /// Create a new location object.
    /// @param sourceIdentifier The source identifier.
    /// @param codeLocation The location in the document.
    explicit Location(SourceIdentifierPtr sourceIdentifier, const unit::CodeLocation codeLocation = {}) noexcept :
        _sourceIdentifier{std::move(sourceIdentifier)}, _codeLocation{codeLocation} {}
    /// Default copy constructor.
    Location(const Location &) = default;
    /// Default move constructor.
    Location(Location &&) = default;
    /// Default copy assignment.
    auto operator=(const Location &) -> Location & = default;
    /// Default move assignment.
    auto operator=(Location &&) -> Location & = default;

public: // operators
    /// Compare this location to another for equality.
    /// @param other The location to compare.
    /// @return `true` if both the source identifier and position are equal.
    auto operator==(const Location &other) const noexcept -> bool {
        return SourceIdentifier::areEqual(_sourceIdentifier, other._sourceIdentifier) &&
            _codeLocation == other._codeLocation;
    }
    /// Compare this location to another for inequality.
    /// @param other The location to compare.
    /// @return `true` if the locations are not equal, `false` otherwise.
    auto operator!=(const Location &other) const noexcept -> bool { return !operator==(other); }

public: // accessors
    /// Test if this location is undefined.
    /// @return `true` if undefined, `false` otherwise.
    [[nodiscard]] auto isUndefined() const noexcept -> bool {
        return _sourceIdentifier == nullptr && _codeLocation.isUndefined();
    }
    /// The source identifier for this location.
    [[nodiscard]] auto sourceIdentifier() const noexcept -> const SourceIdentifierPtr & { return _sourceIdentifier; }
    /// Get the code location.
    [[nodiscard]] constexpr auto codeLocation() const noexcept -> unit::CodeLocation { return _codeLocation; }

public: // conversion
    /// Get this location as a text.
    /// The location is formatted as: (source identifier):(line):(column).
    /// If no source identifier is specified, it is replaced by the text `<unknown>`.
    /// @return A string with this location information.
    [[nodiscard]] auto toText() const -> text::String;

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Location &object) -> impl::InternalViewPtr;
#endif

private:
    SourceIdentifierPtr _sourceIdentifier; ///< The source identifier.
    unit::CodeLocation _codeLocation;      ///< The location in the source code.
};

}
