// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdexcept>

/// Exception thrown when an application instance is accessed outside an open test scope.
class IllegalApplicationInstanceAccess : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    // defaults
    ~IllegalApplicationInstanceAccess() override = default;
};
