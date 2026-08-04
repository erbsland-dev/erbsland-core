// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/CaseSensitivity.hpp"
#include "../../../NamePath.hpp"

#include <utility>
#include <vector>

namespace erbsland::conf::vr::builder {

/// Defines an index of key paths that can be referenced by key constraints.
class KeyIndex : public Attribute {
public:
    /// Creates an unnamed key index.
    /// @param keyPaths The indexed key paths.
    /// @param caseSensitivity The key comparison case sensitivity.
    explicit KeyIndex(
        std::vector<NamePathLike> keyPaths,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        _keyPaths{std::move(keyPaths)}, _caseSensitivity{caseSensitivity} {}

    /// Creates a named key index.
    /// @param name The index name.
    /// @param keyPaths The indexed key paths.
    /// @param caseSensitivity The key comparison case sensitivity.
    KeyIndex(
        Name name,
        std::vector<NamePathLike> keyPaths,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        _name{std::move(name)}, _keyPaths{std::move(keyPaths)}, _caseSensitivity{caseSensitivity} {}

    /// Creates a named key index from text.
    /// @param name The regular index name.
    /// @param keyPaths The indexed key paths.
    /// @param caseSensitivity The key comparison case sensitivity.
    KeyIndex(
        const text::String &name,
        std::vector<NamePathLike> keyPaths,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        KeyIndex(Name::createRegular(name), std::move(keyPaths), caseSensitivity) {}

    /// Creates an unnamed key index for one path.
    /// @param keyPath The indexed key path.
    explicit KeyIndex(const NamePathLike &keyPath) :
        KeyIndex(std::vector<NamePathLike>{keyPath}, text::CaseSensitivity::CaseInsensitive) {}

    /// Creates a named key index for one path.
    /// @param name The index name.
    /// @param keyPath The indexed key path.
    /// @param caseSensitivity The key comparison case sensitivity.
    KeyIndex(
        Name name,
        const NamePathLike &keyPath,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        KeyIndex(std::move(name), std::vector<NamePathLike>{keyPath}, caseSensitivity) {}

    /// Creates a named key index for one path from text.
    /// @param name The regular index name.
    /// @param keyPath The indexed key path.
    /// @param caseSensitivity The key comparison case sensitivity.
    KeyIndex(
        const text::String &name,
        const NamePathLike &keyPath,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        KeyIndex(name, std::vector<NamePathLike>{keyPath}, caseSensitivity) {}

    /// Creates an unnamed key index from a path list.
    /// @param keyPaths The indexed key paths.
    /// @param caseSensitivity The key comparison case sensitivity.
    explicit KeyIndex(
        const std::initializer_list<NamePathLike> keyPaths,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        KeyIndex(std::vector<NamePathLike>{keyPaths}, caseSensitivity) {}

    /// Creates a named key index from a path list.
    /// @param name The index name.
    /// @param keyPaths The indexed key paths.
    /// @param caseSensitivity The key comparison case sensitivity.
    KeyIndex(
        Name name,
        const std::initializer_list<NamePathLike> keyPaths,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        KeyIndex(std::move(name), std::vector<NamePathLike>{keyPaths}, caseSensitivity) {}

    /// Creates a named key index from text and a path list.
    /// @param name The regular index name.
    /// @param keyPaths The indexed key paths.
    /// @param caseSensitivity The key comparison case sensitivity.
    KeyIndex(
        const text::String &name,
        const std::initializer_list<NamePathLike> keyPaths,
        const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseInsensitive) :
        KeyIndex(name, std::vector<NamePathLike>{keyPaths}, caseSensitivity) {}

    void operator()(Rule &rule) override;

    Name _name;
    std::vector<NamePathLike> _keyPaths;
    text::CaseSensitivity _caseSensitivity{text::CaseSensitivity::CaseInsensitive};
};

}
