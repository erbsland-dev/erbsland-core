// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../LogLineFormat.hpp"
#include "../LogWriter_fwd.hpp"
#include "../LogWriterFilter.hpp"

#include <optional>

namespace erbsland::log::impl {

/// One implementation-only writer route.
/// @tested{LogCoreTest LogWriterTest}
struct LogWriterBinding final {
    LogWriterPtr writer;                     ///< Bound writer instance.
    LogWriterFilter filter;                  ///< Route filter applied before delivery.
    std::optional<LogLineFormat> lineFormat; ///< Complete route format, or empty for the configuration default.
};

}
