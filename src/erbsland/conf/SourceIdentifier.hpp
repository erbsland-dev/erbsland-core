// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceIdentifier_fwd.hpp"

#include "impl/utilities/InternalView_fwd.hpp"

#include "../text/String.hpp"

namespace erbsland::conf {

/// Lightweight identifier for a configuration source.
/// Instances of this class are usually shared between locations so that the
/// parser and higher layers can refer to the same source without copying the
/// underlying name and path strings.
class SourceIdentifier {
    class PrivateTag {};

public:
    /// Create a new source identifier with explicit name and path.
    /// @param name The name of the source.
    /// @param path The path of the source.
    SourceIdentifier(text::String name, text::String path, PrivateTag) noexcept;
    /// Factory function to create a shared source identifier.
    /// @param name The source name.
    /// @param path The source path.
    /// @return Shared-pointer to the new instance.
    [[nodiscard]] static auto create(text::String name, text::String path) noexcept -> SourceIdentifierPtr;
    /// Create a new source identifier for a file.
    /// @param path The source path.
    /// @return Shared-pointer to the new instance.
    [[nodiscard]] static auto createForFile(text::String path) noexcept -> SourceIdentifierPtr;
    /// Create a new source identifier for text.
    /// @return Shared-pointer to the new instance.
    [[nodiscard]] static auto createForText() noexcept -> SourceIdentifierPtr;

    // defaults
    ~SourceIdentifier() = default;

public: // operators
    /// Compare this source identifier to another for equality.
    /// @param other The other identifier to compare.
    /// @return `true` if both identifiers have the same name and path.
    [[nodiscard]] auto operator==(const SourceIdentifier &other) const noexcept -> bool {
        return _name == other._name && _path == other._path;
    }
    /// Compare this source identifier to another for inequality.
    /// @param other The other identifier to compare.
    /// @return `true` if the identifiers differ.
    [[nodiscard]] auto operator!=(const SourceIdentifier &other) const noexcept -> bool { return !operator==(other); };

public: // accessors
    /// Get the name of the source.
    [[nodiscard]] auto name() const noexcept -> text::String { return _name; }
    /// Get the path of the source.
    [[nodiscard]] auto path() const noexcept -> text::String { return _path; }

public: // conversion
    /// Get a text representation of this source identifier.
    /// @return A text representation of the identifier.
    [[nodiscard]] auto toText() const noexcept -> text::String;

public: // helpers
    /// A helper function to easily compare two source identifier pointers.
    /// @param a The first identifier.
    /// @param b The second identifier.
    /// @return `true` if both identifier pointers are either nullptr, or compare to the same values.
    static auto areEqual(const SourceIdentifierPtr &a, const SourceIdentifierPtr &b) noexcept -> bool;

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const SourceIdentifier &object) -> impl::InternalViewPtr;
#endif

private:
    text::String _name; ///< The name of the source.
    text::String _path; ///< The path of the source.
};

}
