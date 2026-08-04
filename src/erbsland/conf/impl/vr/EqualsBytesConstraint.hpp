// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EqualsConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate equality of byte-block values.
class EqualsBytesConstraint final : public EqualsConstraint<mem::ByteBlock> {
public:
    /// Creates a byte-block equality constraint from its expected value.
    /// @tparam Fwd A forwarding reference to the expected byte-block type.
    /// @param expected The expected byte block.
    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, mem::ByteBlock>)
    explicit EqualsBytesConstraint(Fwd &&expected) : EqualsConstraint(std::forward<Fwd>(expected)) {}

protected:
    void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const override;
};

}
