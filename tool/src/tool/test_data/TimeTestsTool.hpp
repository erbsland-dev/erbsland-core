// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FixtureTool.hpp"

namespace tool::test_data {

/// Generate independent calendar, wall-clock, and duration expectations.
class TimeTestsTool final : public FixtureTool {
public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    void execute() override;

protected: // implement FixtureTool
    [[nodiscard]] auto defaultDirectory() const -> el::String override;

private:
    /// Prepare date epoch, manipulation, distance, and comparison tables.
    void generateDates();
    /// Prepare midnight offsets and wall-clock comparison tables.
    void generateTimes();
    /// Prepare exact duration comparison and unit conversion tables.
    void generateAmounts();
    /// Prepare datetime epoch, duration arithmetic, and comparison tables.
    void generateDateTimes();
    /// Render and retain one complete fixture table for deferred publication.
    void addTable(const el::String &name, const el::String &schema, const el::render::ValueList &rows);
};

}
