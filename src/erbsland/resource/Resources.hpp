// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceInfo.hpp"
#include "Resources_fwd.hpp"

#include "../mem/ByteBlock.hpp"
#include "../mem/ByteSpan.hpp"
#include "../text/String.hpp"

#include <optional>

namespace erbsland::resource {

/// A thread-safe read-only compiled-resource lookup interface.
/// Borrowed stored-data spans remain valid for the lifetime of this interface.
/// Implementations must permit concurrent calls to every const lookup method.
/// @notest{Abstract interface; ResourceManagerTest covers the compiled-resource implementation.}
class Resources {
public:
    // defaults
    Resources() = default;
    virtual ~Resources() = default;
    Resources(const Resources &) = delete;
    auto operator=(const Resources &) -> Resources & = delete;
    Resources(Resources &&) = delete;
    auto operator=(Resources &&) -> Resources & = delete;

public:
    /// Test whether a resource exists.
    [[nodiscard]] virtual auto contains(const text::String &identifier, const text::String &path) const -> bool = 0;
    /// Get the exact embedded representation.
    [[nodiscard]] virtual auto getStoredData(const text::String &identifier, const text::String &path) const
        -> std::optional<mem::ConstByteSpan> = 0;
    /// Get the exact embedded representation.
    /// @throws ResourceError If the resource does not exist.
    [[nodiscard]] virtual auto getStoredDataOrThrow(const text::String &identifier, const text::String &path) const
        -> mem::ConstByteSpan = 0;
    /// Get the original logical bytes, transparently decompressing when necessary.
    [[nodiscard]] virtual auto getData(const text::String &identifier, const text::String &path) const
        -> std::optional<mem::ByteBlock> = 0;
    /// Get the original logical bytes.
    /// @throws ResourceError If the resource is missing or invalid.
    [[nodiscard]] virtual auto getDataOrThrow(const text::String &identifier, const text::String &path) const
        -> mem::ByteBlock = 0;
    /// Get the exact logical bytes as tolerant UTF-8 text.
    [[nodiscard]] virtual auto getText(const text::String &identifier, const text::String &path) const
        -> std::optional<text::String> = 0;
    /// Get the exact logical bytes as tolerant UTF-8 text.
    /// @throws ResourceError If the resource is missing or invalid.
    [[nodiscard]] virtual auto getTextOrThrow(const text::String &identifier, const text::String &path) const
        -> text::String = 0;
    /// Get resource metadata.
    [[nodiscard]] virtual auto getInfo(const text::String &identifier, const text::String &path) const
        -> std::optional<ResourceInfo> = 0;
    /// Get resource metadata.
    /// @throws ResourceError If the resource does not exist.
    [[nodiscard]] virtual auto getInfoOrThrow(const text::String &identifier, const text::String &path) const
        -> ResourceInfo = 0;
};

}
