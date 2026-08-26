// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CatalogServer.hpp"
#include "CatalogStorage.hpp"

#include <memory>

namespace demo {

/// Concrete server part that starts after catalog storage and accesses it through its public interface.
class CatalogServerPart final : public el::ApplicationPartWithInterface<CatalogServer> {
public:
    /// Declare the services that must be running before this part starts.
    [[nodiscard]] static auto dependencies() -> el::ApplicationPartIdentifierList {
        return el::ApplicationPartIdentifierList{CatalogStorage::partIdentifier()};
    }
    /// Create one server part when the manager prepares the dependency graph.
    [[nodiscard]] static auto create() -> std::shared_ptr<CatalogServerPart> {
        return std::make_shared<CatalogServerPart>();
    }

public: // implement CatalogServer
    [[nodiscard]] auto endpoint() const -> el::String override { return "local catalog endpoint"_el; }

protected: // implement ApplicationPart
    void initialize() override {
        _storage = partManager().part<CatalogStorage>();
        el::io::printLine("catalog server initialized; "_el, _storage->status());
    }
    void cleanup() noexcept override {
        _storage.reset();
        el::io::printLine("catalog server cleaned up"_el);
    }

private:
    std::shared_ptr<CatalogStorage> _storage;
};

}
