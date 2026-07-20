// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../cryptology/HashAlgorithm.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::conf::impl::defaults {

using namespace text::literals;

/// The hash algorithm to use for the document hash.
constexpr auto documentHashAlgorithm = cryptology::HashAlgorithm{cryptology::HashAlgorithm::Sha3_256};

/// The identifier for text sources.
constexpr auto textSourceIdentifier = "text"_el;

/// The identifier for file sources.
constexpr auto fileSourceIdentifier = "file"_el;

/// The identifier for name paths.
constexpr auto namePathIdentifier = "name-path"_el;

/// The configuration language version.
constexpr auto languageVersion = "1.0"_el;

/// The default file suffix.
constexpr auto fileSuffix = ".elcl"_el;

}
