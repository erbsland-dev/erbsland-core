// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsProtocolError.hpp"

#include <utility>

namespace erbsland::network::impl {

TlsProtocolError::TlsProtocolError(const TlsAlertDescription alert, text::String reason) noexcept :
    err::RuntimeError{std::move(reason)}, _alert{alert} {
}

}
