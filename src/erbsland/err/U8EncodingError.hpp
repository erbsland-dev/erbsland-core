// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingError.hpp"

#include "../unit/ByteIndex.hpp"

namespace erbsland::err {

/// A UTF-8 encoding error exception.
class U8EncodingError final : public EncodingError {
public:
    /// Create a UTF-8 encoding error exception with a reason.
    /// @param reason The reason for the encoding error.
    /// @param index The byte index where the error occurred.
    explicit U8EncodingError(const std::string_view reason, const unit::ByteIndex index) noexcept :
        EncodingError{reason}, _index(index) {}

    // defaults
    ~U8EncodingError() override = default;

private:
    unit::ByteIndex _index;
};

}
