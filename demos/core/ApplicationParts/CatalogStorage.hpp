// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Public service interface for the catalog storage part.
class CatalogStorage {
public:
    // defaults
    virtual ~CatalogStorage() = default;

public:
    /// Return the stable identifier shared by clients and the implementation.
    [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
        static const auto result = el::ApplicationPartIdentifier::create("dev.erbsland.demo.catalog-storage"_el);
        return result;
    }
    /// Describe the prepared storage service.
    [[nodiscard]] virtual auto status() const -> el::String = 0;
};

}
