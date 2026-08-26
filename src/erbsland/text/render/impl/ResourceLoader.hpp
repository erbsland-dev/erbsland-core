// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ResourceLoader.hpp"

#include "../../../resource/Resources_fwd.hpp"

namespace erbsland::text::render::impl {

/// Resource-provider implementation of the public layout loader.
/// @tested{ResourceLoaderTest}
class ResourceLoader final : public render::ResourceLoader {
public:
    /// Create a retained or application-backed resource loader.
    ResourceLoader(resource::ResourcesConstPtr resources, String identifier, String pathPrefix);

    // defaults/deletions
    ~ResourceLoader() override = default;
    ResourceLoader(const ResourceLoader &) = delete;
    ResourceLoader(ResourceLoader &&) = delete;
    auto operator=(const ResourceLoader &) -> ResourceLoader & = delete;
    auto operator=(ResourceLoader &&) -> ResourceLoader & = delete;

public: // implement Loader
    [[nodiscard]] auto load(const String &layout) -> std::optional<LayoutSource> override;

private:
    /// Validate the portable resource identifier.
    static void validateIdentifier(const String &identifier);
    /// Validate the optional normalized portable path prefix.
    static void validatePathPrefix(const String &pathPrefix);
    /// Build the exact resource path for a logical layout name.
    [[nodiscard]] auto resourcePath(const String &layout) const -> String;

private:
    resource::ResourcesConstPtr _retainedResources; ///< Explicit provider ownership, if supplied.
    const resource::Resources *_resources{};        ///< Active custom or application provider.
    String _identifier;                             ///< Exact compiled-resource identifier.
    String _pathPrefix;                             ///< Optional normalized resource path prefix.
};

}
