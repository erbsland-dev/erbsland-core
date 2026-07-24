// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLineStatus.hpp"

#include "../text/String.hpp"
#include "../util/ResultWithData.hpp"

namespace erbsland::cterm {

/// An interactive read-line result with committed text.
/// Only `ReadLineStatus::Committed` transports text; all other states transport an empty string.
using ReadLineResult = util::ResultWithData<text::String, ReadLineStatus>;

}
