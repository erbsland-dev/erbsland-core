// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SteadyTimeSource.hpp"

namespace erbsland::profiling {

auto SteadyTimeSource::now() noexcept -> time::TimePoint {
    return time::TimePoint::now();
}

}
