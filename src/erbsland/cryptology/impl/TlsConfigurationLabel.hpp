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

/// Validate one hierarchical TLS configuration label.
/// @param label The label to validate, or an empty label for the global default.
/// @throws err::ParameterError If the label is invalid.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
void validate(const text::String &label);

}
