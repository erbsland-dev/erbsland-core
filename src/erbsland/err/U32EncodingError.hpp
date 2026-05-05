// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingError.hpp"

#include "../unit/CpIndex.hpp"

namespace erbsland::err {

/// A UTF-32 encoding error exception.
class U32EncodingError final : public EncodingError {
public:
    /// Create a UTF-32 encoding error exception with a reason.
    /// @param reason The reason for the encoding error.
    /// @param index The code-point index where the error occurred.
    explicit U32EncodingError(const std::string_view reason, const unit::CpIndex index) noexcept :
        EncodingError{reason}, _index(index) {}

private:
    unit::CpIndex _index;
};

}
