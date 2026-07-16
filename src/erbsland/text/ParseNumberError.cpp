// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParseNumberError.hpp"

#include "String.hpp"

namespace erbsland::text {

ParseNumberError::ParseNumberError(
    const std::string_view reason, const ReadNumberStatus status, const unit::CpIndex position) noexcept :
    ParseNumberError{String{reason}, status, position} {
}

}
