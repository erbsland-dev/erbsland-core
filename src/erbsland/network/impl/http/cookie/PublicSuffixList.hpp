// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"

namespace erbsland::network::impl::public_suffix {

/// Test whether the complete canonical IDNA ASCII host name is a public suffix.
/// @tested{PublicSuffixListTest HttpCookieJarTest HttpClientTest}
[[nodiscard]] auto isPublicSuffix(const text::String &host) -> bool;
/// Return the registrable domain, or empty text for a public suffix or invalid input.
/// @tested{PublicSuffixListTest HttpCookieJarTest HttpClientTest}
[[nodiscard]] auto registrableDomain(const text::String &host) -> text::String;

}
