// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CatalogStorage.hpp"

#include <memory>

namespace demo {

/// Concrete application part providing catalog storage.
class CatalogStoragePart final : public el::ApplicationPartWithInterface<CatalogStorage> {
public:
    /// Create one storage part when the manager prepares the dependency graph.
    [[nodiscard]] static auto create() -> std::shared_ptr<CatalogStoragePart> {
        return std::make_shared<CatalogStoragePart>();
    }

public: // implement CatalogStorage
    [[nodiscard]] auto status() const -> el::String override { return "catalog storage is ready"_el; }

protected: // implement ApplicationPart
    void initialize() override { el::io::printLine("catalog storage initialized"_el); }
    void cleanup() noexcept override { el::io::printLine("catalog storage cleaned up"_el); }
};

}
