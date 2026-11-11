// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteCount.hpp>
#include <erbsland/LogTraceSection.hpp>
#include <erbsland/StringLiteral.hpp>

namespace demo {

using namespace el::text::literals;

/// Reserved name of the SHA3-256 manifest within each package.
inline constexpr auto cChecksumFileName = "checksum.txt"_el;
/// The identifier for the verbose logging option.
constexpr static auto cVerboseLoggingOption = "verbose"_el;
/// The trace section for verbose logging.
constexpr static auto cVerboseTraceSection = el::LogTraceSection{"verbose"_el};
/// The identifier for the source directory option.
constexpr static auto cSourceDirectoryOption = "source"_el;
/// The identifier for the output directory option.
constexpr static auto cOutputDirectoryOption = "output"_el;

}
