// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Public service interface for the catalog server part.
class CatalogServer {
public:
    // defaults
    virtual ~CatalogServer() = default;

public:
    /// Return the stable identifier shared by clients and the implementation.
    [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
        static const auto result = el::ApplicationPartIdentifier::create("dev.erbsland.demo.catalog-server"_el);
        return result;
    }
    /// Return the endpoint represented by this demo service.
    [[nodiscard]] virtual auto endpoint() const -> el::String = 0;
};

}
