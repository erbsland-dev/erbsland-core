// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceManagerData_fwd.hpp"
#include "ResourceStorageInfo.hpp"
#include "ResourceStorageRegistration.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../text/StringHashMap.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace erbsland::resource::impl {

/// Owns the lazy compiled-resource index and decoded-value caches.
/// @tested{ResourceManagerTest}
class ResourceManagerData final {
public:
    /// Cached state for one immutable compiled resource.
    struct EntryState final {
        /// Create cached state from parsed generated metadata.
        explicit EntryState(ResourceStorageInfo entry) : entry{std::move(entry)} {}

        ResourceStorageInfo entry;          ///< Parsed and cached storage information.
        std::mutex mutex;                   ///< Protects lazy decoded values.
        std::optional<mem::ByteBlock> data; ///< Cached logical bytes.
        std::optional<text::String> text;   ///< Cached logical text.
    };
    /// Shared state pointer used by the immutable index.
    using EntryStatePtr = std::shared_ptr<EntryState>;
    /// Path-to-entry index for one identifier.
    using PathMap = text::StringHashMap<EntryStatePtr>;

public:
    /// Build an index from every statically registered collection.
    ResourceManagerData();

    // defaults/deletions
    ~ResourceManagerData() = default;
    ResourceManagerData(const ResourceManagerData &) = delete;
    auto operator=(const ResourceManagerData &) -> ResourceManagerData & = delete;

public:
    /// Find one cached entry state.
    [[nodiscard]] auto find(const text::String &identifier, const text::String &path) const -> EntryStatePtr;

private:
    text::StringHashMap<PathMap> _index; ///< Two-level exact identifier/path index.
};

}
