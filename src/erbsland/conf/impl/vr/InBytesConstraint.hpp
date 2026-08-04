// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate that a byte block is one of the configured values.
class InBytesConstraint final : public InConstraint<mem::ByteBlock> {
public:
    /// Creates a byte-block membership constraint.
    /// @param values The allowed byte-block values.
    explicit InBytesConstraint(std::vector<mem::ByteBlock> values) : InConstraint(std::move(values)) {}

protected:
    void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const override;
};

}
