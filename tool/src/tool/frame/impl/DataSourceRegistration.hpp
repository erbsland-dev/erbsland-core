// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../DataSource.hpp"

#include <erbsland/String.hpp>

#include <memory>

namespace tool::frame::impl {

/// Retain a concrete source's static identity and initialization state.
/// @tested{DataSourceTest}
struct DataSourceRegistration final {
    el::String identifier;              ///< Identity provided by the concrete source type.
    std::unique_ptr<DataSource> source; ///< Single application-owned instance.
    el::conf::ValuePtr configuration;   ///< Validated settings retained until initialization.
    bool initialized = false;           ///< True only after initialization returned successfully.
};

}
