// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamReadStatus.hpp"

#include "../util/ResultWithData.hpp"

namespace erbsland::stream {

/// A bounded stream read result with transported data.
/// It derives from `StreamReadStatus`, so use typed predicates or compare it directly with that status. Only `Data`
/// has a caller-visible payload; `Timeout` and `Finished` transport the default/empty data value.
/// @tparam tData The transported data type.
/// @tested{StreamResultTest}
template <typename tData>
using StreamReadResult = util::ResultWithData<tData, StreamReadStatus>;

}
