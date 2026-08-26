// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Loader.hpp"

#include "../../resource/Resources_fwd.hpp"

namespace erbsland::text::render {

/// A layout loader backed by one compiled-resource identifier and optional path prefix.
/// A null resource provider selects the application resource manager. Do not create an application-backed loader
/// during unsafe static initialization.
/// @seedoc{/reference/text/render}
/// @tested{ResourceLoaderTest}
class ResourceLoader : public Loader {
public:
    /// Create a loader using the application resource manager.
    /// @param identifier The exact portable resource identifier.
    /// @param pathPrefix The optional normalized relative resource path prefix.
    /// @return A new resource layout loader.
    [[nodiscard]] static auto create(String identifier, String pathPrefix = {}) -> LoaderPtr;
    /// Create a loader retaining an explicit provider, or using application resources when it is null.
    /// @param resources The retained resource provider, or null for application resources.
    /// @param identifier The exact portable resource identifier.
    /// @param pathPrefix The optional normalized relative resource path prefix.
    /// @return A new resource layout loader.
    [[nodiscard]] static auto create(resource::ResourcesConstPtr resources, String identifier, String pathPrefix = {})
        -> LoaderPtr;
};

}
