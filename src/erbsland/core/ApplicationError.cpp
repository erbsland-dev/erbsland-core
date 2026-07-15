// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationError.hpp"

#include "impl/ApplicationErrorDiagnostic.hpp"

#include "../text/StringFormat.hpp"

namespace erbsland::core {

auto ApplicationError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::ApplicationErrorDiagnostic>(_context);
}

}
