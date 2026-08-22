// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceManagerData_fwd.hpp"

#include <atomic>
#include <cstdint>
#include <span>

namespace erbsland::resource::impl {

/// A provider for one opaque compiled-resource data block.
using ResourceDataProvider = auto (*)() noexcept -> std::span<const std::uint8_t>;

/// Registers separated generated data and metadata blocks without interpreting either block.
/// This implementation API is used exclusively by generated resource source files.
/// @tested{ResourceManagerTest}
class ResourceStorageRegistration final {
    friend class ResourceManagerData;

private:
    /// One node in the process-wide compiled-resource registry.
    struct Node final {
        ResourceDataProvider dataProvider;       ///< Provider for the opaque stored data.
        std::span<const std::uint8_t> infoBlock; ///< Encoded metadata stored separately from the data.
        Node *next{};                            ///< Next registry node.
    };

public:
    /// Register separated static data and metadata blocks.
    ResourceStorageRegistration(ResourceDataProvider dataProvider, std::span<const std::uint8_t> infoBlock) noexcept;

    // defaults/deletions
    ~ResourceStorageRegistration() = default;
    ResourceStorageRegistration(const ResourceStorageRegistration &) = delete;
    ResourceStorageRegistration(ResourceStorageRegistration &&) = delete;
    auto operator=(const ResourceStorageRegistration &) -> ResourceStorageRegistration & = delete;
    auto operator=(ResourceStorageRegistration &&) -> ResourceStorageRegistration & = delete;

private:
    /// Access the process-wide registry head.
    [[nodiscard]] static auto registry() noexcept -> std::atomic<Node *> &;

private:
    Node _node; ///< This registration's intrusive registry node.
};

}
