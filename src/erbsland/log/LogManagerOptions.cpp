// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogManagerOptions.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"

namespace erbsland::log {

using namespace err;
using namespace text::literals;

auto LogManagerOptions::setMaximumEntries(const std::size_t value) -> LogManagerOptions & {
    if (value == 0U) {
        throw ParameterFailure{"The log queue entry limit must be positive."_el, "value"_el};
    }
    _maximumEntries = value;
    return *this;
}

auto LogManagerOptions::setMaximumBytes(const unit::ByteCount value) -> LogManagerOptions & {
    if (value.isZero()) {
        throw ParameterFailure{"The log queue byte limit must be positive."_el, "value"_el};
    }
    _maximumBytes = value;
    return *this;
}

auto LogManagerOptions::setReservedErrorEntries(const std::size_t value) noexcept -> LogManagerOptions & {
    _reservedErrorEntries = value;
    return *this;
}

auto LogManagerOptions::setReservedErrorBytes(const unit::ByteCount value) noexcept -> LogManagerOptions & {
    _reservedErrorBytes = value;
    return *this;
}

auto LogManagerOptions::setMaximumMessageBytes(const unit::ByteCount value) -> LogManagerOptions & {
    if (value.isZero()) {
        throw ParameterFailure{"The log message limit must be positive."_el, "value"_el};
    }
    _maximumMessageBytes = value;
    return *this;
}

auto LogManagerOptions::setShutdownTimeout(const time::TimeDelta value) -> LogManagerOptions & {
    if (!value.isPositive()) {
        throw ParameterFailure{"The log shutdown timeout must be positive."_el, "value"_el};
    }
    _shutdownTimeout = value;
    return *this;
}

}
