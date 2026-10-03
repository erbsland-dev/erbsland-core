// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl::tls_configuration_label {

/// Maximum byte length of one label.
inline constexpr auto cMaximumLength = std::size_t{255U};
/// Maximum number of slash-delimited segments in one non-empty label.
inline constexpr auto cMaximumSegments = std::size_t{16U};

/// Get the validation error for a hierarchical TLS configuration label.
/// @param label The label to validate; empty selects the global default.
/// @return The reason the label is invalid, or an empty string for a valid label.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
[[nodiscard]] auto validationError(const text::String &label) -> text::String;

/// Validate one hierarchical TLS configuration label.
/// @param label The TLS configuration label, or empty for the global default. Must be at most 255 bytes with at most 16
/// non-empty `/`-separated segments using `a-z`, `0-9`, and `-`; segments must not start or end with `-`.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
void validate(const text::String &label);

}
