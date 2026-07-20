// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

template <typename tValue, typename tArgument>
struct FormatAs;

template <typename T>
struct FormatAsInt64;

template <typename T>
struct FormatAsUInt64;

template <typename T>
struct FormatAsDouble;

template <typename T>
struct FormatAsBool;

template <typename T>
struct FormatAsChar;

template <typename T>
struct FormatAsText;

template <typename T>
struct FormatAsU8Text;

template <typename T>
struct FormatAsU16Text;

template <typename T>
struct FormatAsU32Text;

template <typename T>
struct FormatAsBytes;

}
