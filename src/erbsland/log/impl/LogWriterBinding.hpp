// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../LogWriter_fwd.hpp"
#include "../LogWriterFilter.hpp"

namespace erbsland::log::impl {

/// One implementation-only writer route.
/// @tested{LogCoreTest LogWriterTest}
struct LogWriterBinding final {
    LogWriterPtr writer;    ///< Bound writer instance.
    LogWriterFilter filter; ///< Route filter applied before delivery.
};

}
