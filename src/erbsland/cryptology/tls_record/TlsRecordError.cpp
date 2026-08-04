// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsRecordError.hpp"

#include <utility>

namespace erbsland::cryptology {

TlsRecordError::TlsRecordError(const TlsRecordErrorCategory category, text::String reason) noexcept :
    err::RuntimeError{std::move(reason)}, _category{category} {
}

}
