// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParseNumberError.hpp"

#include "String.hpp"
#include "StringEditor.hpp"

namespace erbsland::text {

ParseNumberError::ParseNumberError(String reason, ReadNumberStatus status, unit::CpIndex position) noexcept :
    err::ParseError{std::move(reason), position}, _status{status} {
}

}
