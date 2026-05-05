// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingError.hpp"

#include "../unit/U16DataIndex.hpp"

namespace erbsland::err {

/// A UTF-16 encoding error exception.
class U16EncodingError final : public EncodingError {
public:
    /// Create a UTF-16 encoding error exception with a reason.
    /// @param reason The reason for the encoding error.
    /// @param index The word index where the error occurred.
    explicit U16EncodingError(const std::string_view reason, const unit::U16DataIndex index) noexcept :
        EncodingError{reason}, _index(index) {}

private:
    unit::U16DataIndex _index;
};

}
