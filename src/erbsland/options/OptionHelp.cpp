// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionHelp.hpp"

namespace erbsland::options {

OptionHelp::OptionHelp(text::String description) : _description{std::move(description)} {
}

}
