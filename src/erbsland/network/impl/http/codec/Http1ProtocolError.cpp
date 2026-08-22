// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1ProtocolError.hpp"

namespace erbsland::network::impl {

Http1ProtocolError::Http1ProtocolError(const Http1FailureReason reason, text::String message) noexcept :
    RuntimeError{std::move(message)}, _reason{reason} {
}

}
