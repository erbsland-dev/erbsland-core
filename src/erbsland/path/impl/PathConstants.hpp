// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/Literals.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::path::impl {

using namespace text::literals;

constexpr auto cMaximumPathCharacters = unit::CpLength{8192U};
constexpr auto cMaximumPathElements = unit::ItemCount{1000U};

constexpr auto cSlash = "/"_el;
constexpr auto cDoubleSlash = "//"_el;
constexpr auto cDriveRootSuffix = ":/"_el;
constexpr auto cWindowsSeparator = "\\"_el;
constexpr auto cWindowsDevicePathRoot = "//."_el;
constexpr auto cWindowsDevicePathPrefix = "//./"_el;
constexpr auto cWindowsExtendedPathPrefix = "//?/"_el;
constexpr auto cWindowsExtendedUncPathPrefix = "//?/UNC/"_el;
constexpr auto cWindowsExtendedNativePathPrefix = "\\\\?\\"_el;
constexpr auto cWindowsExtendedNativeUncPathPrefix = "\\\\?\\UNC"_el;
constexpr auto cWindowsNtPathRoot = "/??"_el;
constexpr auto cWindowsNtPathPrefix = "/??/"_el;

}
