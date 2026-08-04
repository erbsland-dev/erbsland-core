// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextInputStream.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::stream {

using namespace text::literals;
using text::String;
using util::CoAsyncGenerator;
using util::CoTask;

auto TextInputStream::read() -> StreamReadResult<String> {
    return read(cDefaultTextReadMaximum);
}

auto TextInputStream::readLine() -> StreamReadResult<String> {
    return readLine(cDefaultTextReadMaximum);
}

auto TextInputStream::readAll() -> StreamReadResult<String> {
    return readAll(cDefaultTextReadMaximum);
}

auto TextInputStream::coRead() -> CoTask<StreamReadResult<String>> {
    return coRead(cDefaultTextReadMaximum);
}

auto TextInputStream::coRead(const unit::CpLength maximum) -> CoTask<StreamReadResult<String>> {
    auto self = std::static_pointer_cast<TextInputStream>(sharedInputStream());
    return CoTask<StreamReadResult<String>>::run(
        [self = std::move(self), maximum]() -> StreamReadResult<String> { return self->read(maximum); });
}

auto TextInputStream::coReadLine() -> CoTask<StreamReadResult<String>> {
    return coReadLine(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadLine(const unit::CpLength maximum) -> CoTask<StreamReadResult<String>> {
    auto self = std::static_pointer_cast<TextInputStream>(sharedInputStream());
    return CoTask<StreamReadResult<String>>::run(
        [self = std::move(self), maximum]() -> StreamReadResult<String> { return self->readLine(maximum); });
}

auto TextInputStream::coReadAll() -> CoTask<StreamReadResult<String>> {
    return coReadAll(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadAll(const unit::CpLength maximum) -> CoTask<StreamReadResult<String>> {
    auto self = std::static_pointer_cast<TextInputStream>(sharedInputStream());
    return CoTask<StreamReadResult<String>>::run(
        [self = std::move(self), maximum]() -> StreamReadResult<String> { return self->readAll(maximum); });
}

auto TextInputStream::coReadBlocks() -> CoAsyncGenerator<StreamReadResult<String>> {
    return coReadBlocks(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadBlocks(const unit::CpLength maximum) -> CoAsyncGenerator<StreamReadResult<String>> {
    if (maximum.isInfinite() || maximum.isZero()) {
        throw err::ParameterError{"The coroutine text-block length must be positive and finite."_el, "maximum"_el};
    }
    while (true) {
        auto result = co_await coRead(maximum);
        if (result == StreamReadStatus::Finished) {
            co_return;
        }
        if (result == StreamReadStatus::Data && result.data().isEmpty()) {
            continue;
        }
        co_yield std::move(result);
    }
}

auto TextInputStream::coReadLines() -> CoAsyncGenerator<StreamReadResult<String>> {
    return coReadLines(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadLines(const unit::CpLength maximum) -> CoAsyncGenerator<StreamReadResult<String>> {
    if (maximum.isInfinite() || maximum.isZero()) {
        throw err::ParameterError{"The coroutine line length must be positive and finite."_el, "maximum"_el};
    }
    while (true) {
        auto result = co_await coReadLine(maximum);
        if (result == StreamReadStatus::Finished) {
            co_return;
        }
        co_yield std::move(result);
    }
}

}
