// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteCompressionError.hpp"

#include "../text/String.hpp"

#include <utility>

namespace erbsland::mem {

ByteCompressionError::ByteCompressionError(const ByteCompressionErrorReason reason, text::String message) noexcept :
    err::RuntimeError{std::move(message)}, _reason{reason} {
}

ByteCompressionError::ByteCompressionError(
    const ByteCompressionErrorReason reason, const std::string_view message) noexcept :
    err::RuntimeError{message}, _reason{reason} {
}

}
