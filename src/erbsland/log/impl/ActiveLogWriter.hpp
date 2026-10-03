// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLineFormatter_fwd.hpp"

#include "../LogWriter_fwd.hpp"
#include "../LogWriterFilter.hpp"

namespace erbsland::log::impl {

/// A runtime writer route with its filter and shared compiled formatter.
/// @tested{LogRouteFormatTest ActiveLogConfigurationTest}
struct ActiveLogWriter final {
    LogWriterPtr writer;                ///< Destination receiving accepted entries.
    LogWriterFilter filter;             ///< Filter evaluated before formatting and delivery.
    LogLineFormatterConstPtr formatter; ///< Nonempty compiled formatter shared by routes using the default format.
};

}
