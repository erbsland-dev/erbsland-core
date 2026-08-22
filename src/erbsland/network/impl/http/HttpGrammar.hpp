// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/CharSet.hpp"
#include "../../../text/String.hpp"

namespace erbsland::network::impl::http_grammar {

/// Get the HTTP token character set.
/// This *does not* include the replacement character, what makes text with encoding errors invalid.
/// @tested{HttpValueTest HttpHeadersTest}
[[nodiscard]] auto tokenCharacters() noexcept -> const text::CharSet &;

/// Shared grammar operations for HTTP value types.
/// @tested{HttpValueTest HttpHeadersTest}
[[nodiscard]] auto isToken(const text::String &value) noexcept -> bool;

/// Test whether text is a valid HTTP field value.
/// This *does* include the replacement character, what makes text with UTF-8 encoding errors valid.
/// So, if for unknown reasons a field contains latin1 characters, this still is a valid HTTP field value.
/// For all relevant fields, we can expect visible ASCII characters and ignore custom ones.
/// @tested{HttpValueTest HttpHeadersTest}
[[nodiscard]] auto isFieldValue(const text::String &value) noexcept -> bool;

/// Test whether text is a strict ASCII HTTP request-target.
/// @tested{HttpMessageTest Http1CodecTest}
[[nodiscard]] auto isRequestTarget(const text::String &value) noexcept -> bool;

/// Test whether text is a valid exact HTTP reason phrase.
/// @tested{HttpMessageTest Http1CodecTest}
[[nodiscard]] auto isReasonPhrase(const text::String &value) noexcept -> bool;

/// Compare two validated HTTP tokens with ASCII case folding.
/// @tested{HttpValueTest HttpHeadersTest}
[[nodiscard]] auto equalTokenCI(const text::String &left, const text::String &right) noexcept -> bool;

}
