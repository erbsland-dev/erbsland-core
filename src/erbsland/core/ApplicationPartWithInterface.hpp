// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartTraits.hpp"

namespace erbsland::core {

/// An application-part base implementing one public interface.
/// @tparam T The abstract part interface.
/// @tested{ApplicationPartManagerTest}
template <ApplicationPartInterface T>
class ApplicationPartWithInterface : public ApplicationPart, public T {
public:
    using T::partIdentifier;

    // defaults
    ~ApplicationPartWithInterface() override = default;
};

}
