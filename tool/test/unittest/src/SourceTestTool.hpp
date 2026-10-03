// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NoConfigurationTool.hpp"

/// Declare the sources exercised by a test while retaining the fixture's execution callback.
/// @notest{Unit-test fixture.}
class SourceTestTool : public NoConfigurationTool {
public: // implement Tool
    [[nodiscard]] auto usedDataSources() const -> el::StringList override { return identifiers; }

public:
    inline static el::StringList identifiers; ///< Dependencies selected by the surrounding fixture.
};
