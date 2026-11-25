// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Input.hpp"
#include "Settings.hpp"

#include <erbsland/List.hpp>

namespace tool::security_hashes {

/// Collect unique selected source files without following links.
class Inventory final {
public:
    /// Bind a selection and file service for inventory collection.
    /// @param settings Validated source selection, retained by reference.
    /// @param files Project confinement service, retained by reference.
    Inventory(const Settings &settings, const frame::ProjectFiles &files);

public: // discovery
    /// Collect a deterministic inventory before starting workers.
    /// @return Unique source files sorted by their manifest path.
    /// @throws el::ApplicationError If a selected input is unsafe or inaccessible.
    [[nodiscard]] auto collect() const -> el::List<Input>;

private:
    const Settings &_settings;         ///< Source selection for this inventory.
    const frame::ProjectFiles &_files; ///< Project confinement service.
};

}
