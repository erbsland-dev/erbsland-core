// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Attribute.hpp"

#include "../../../ConfError.hpp"

namespace erbsland::conf::vr::builder {

void Attribute::throwValidationError(text::String message) {
    throw conf::ConfError{ConfErrorCategory::Validation, std::move(message)};
}

}
