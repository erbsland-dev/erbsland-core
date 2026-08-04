// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyElementEqual.hpp"
#include "KeyElementHash.hpp"
#include "KeyEqual.hpp"
#include "KeyHash.hpp"
#include "KeyIndexDataMultiple.hpp"
#include "KeyIndexDataSingle.hpp"

namespace erbsland::conf::impl {

using KeyHashCaseInsensitive = KeyHash<text::CaseSensitivity::CaseInsensitive>;
using KeyHashCaseSensitive = KeyHash<text::CaseSensitivity::CaseSensitive>;
using KeyElementHashCaseInsensitive = KeyElementHash<text::CaseSensitivity::CaseInsensitive>;
using KeyElementHashCaseSensitive = KeyElementHash<text::CaseSensitivity::CaseSensitive>;
using KeyEqualCaseInsensitive = KeyEqual<text::CaseSensitivity::CaseInsensitive>;
using KeyEqualCaseSensitive = KeyEqual<text::CaseSensitivity::CaseSensitive>;
using KeyElementEqualCaseInsensitive = KeyElementEqual<text::CaseSensitivity::CaseInsensitive>;
using KeyElementEqualCaseSensitive = KeyElementEqual<text::CaseSensitivity::CaseSensitive>;
using KeyIndexDataSingleCaseInsensitive = KeyIndexDataSingle<KeyHashCaseInsensitive, KeyEqualCaseInsensitive>;
using KeyIndexDataSingleCaseSensitive = KeyIndexDataSingle<KeyHashCaseSensitive, KeyEqualCaseSensitive>;
using KeyIndexDataMultipleCaseInsensitive = KeyIndexDataMultiple<
    KeyHashCaseInsensitive,
    KeyEqualCaseInsensitive,
    KeyElementHashCaseInsensitive,
    KeyElementEqualCaseInsensitive>;
using KeyIndexDataMultipleCaseSensitive = KeyIndexDataMultiple<
    KeyHashCaseSensitive,
    KeyEqualCaseSensitive,
    KeyElementHashCaseSensitive,
    KeyElementEqualCaseSensitive>;

}
