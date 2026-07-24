// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FakeTimeSource.hpp"

FakeTimeSource::FakeTimeSource(const el::time::TimeDelta step) : _step{step} {
}

auto FakeTimeSource::now() noexcept -> el::time::TimePoint {
    const auto result = _current;
    _current += _step;
    return result;
}
