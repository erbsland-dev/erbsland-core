// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationResourceData_fwd.hpp"

#include "../../../resource/ResourceManager.hpp"

namespace erbsland::core::impl {

/// Application-wide compiled-resource manager.
/// @tested{ApplicationResourceTest}
class ApplicationResourceData final {
public:
    /// Access the compiled resources.
    [[nodiscard]] auto resources() const noexcept -> const resource::Resources & { return _manager; }

private:
    resource::ResourceManager _manager; ///< Compiled-resource manager.
};

}
