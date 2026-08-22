// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceManager_fwd.hpp"
#include "Resources.hpp"

#include "impl/ResourceManagerData_fwd.hpp"

#include <memory>
#include <mutex>

namespace erbsland::resource {

/// The default lazy manager for process-wide compiled resources.
/// Construction performs no allocation. The immutable lookup index and decoded values are created on first access.
/// @seedoc{/reference/resource/resources}
/// @tested{ResourceManagerTest ApplicationResourceTest}
class ResourceManager final : public Resources {
public:
    /// Create an empty manager that initializes its index on first access.
    ResourceManager();
    /// Release cached compiled-resource values.
    ~ResourceManager() override;

    // defaults/deletions
    ResourceManager(const ResourceManager &) = delete;
    auto operator=(const ResourceManager &) -> ResourceManager & = delete;
    ResourceManager(ResourceManager &&) = delete;
    auto operator=(ResourceManager &&) -> ResourceManager & = delete;

public: // implement Resources
    [[nodiscard]] auto contains(const text::String &identifier, const text::String &path) const -> bool override;
    [[nodiscard]] auto getStoredData(const text::String &identifier, const text::String &path) const
        -> std::optional<mem::ConstByteSpan> override;
    [[nodiscard]] auto getStoredDataOrThrow(const text::String &identifier, const text::String &path) const
        -> mem::ConstByteSpan override;
    [[nodiscard]] auto getData(const text::String &identifier, const text::String &path) const
        -> std::optional<mem::ByteBlock> override;
    [[nodiscard]] auto getDataOrThrow(const text::String &identifier, const text::String &path) const
        -> mem::ByteBlock override;
    [[nodiscard]] auto getText(const text::String &identifier, const text::String &path) const
        -> std::optional<text::String> override;
    [[nodiscard]] auto getTextOrThrow(const text::String &identifier, const text::String &path) const
        -> text::String override;
    [[nodiscard]] auto getInfo(const text::String &identifier, const text::String &path) const
        -> std::optional<ResourceInfo> override;
    [[nodiscard]] auto getInfoOrThrow(const text::String &identifier, const text::String &path) const
        -> ResourceInfo override;

private:
    /// Access the lazily initialized manager data.
    [[nodiscard]] auto data() const -> impl::ResourceManagerData &;

private:
    mutable std::mutex _mutex;                                ///< Protects lazy data creation.
    mutable std::unique_ptr<impl::ResourceManagerData> _data; ///< Lazy lookup and cache data.
};

}
