// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextInputStream.hpp"

#include "../err/ParameterError.hpp"

namespace erbsland::stream {

auto TextInputStream::read() -> StreamReadResult<text::String> {
    return read(cDefaultTextReadMaximum);
}

auto TextInputStream::readLine() -> StreamReadResult<text::String> {
    return readLine(cDefaultTextReadMaximum);
}

auto TextInputStream::readAll() -> StreamReadResult<text::String> {
    return readAll(cDefaultTextReadMaximum);
}

auto TextInputStream::coRead() -> util::CoTask<StreamReadResult<text::String>> {
    return coRead(cDefaultTextReadMaximum);
}

auto TextInputStream::coRead(const unit::CpLength maximum) -> util::CoTask<StreamReadResult<text::String>> {
    auto self = std::static_pointer_cast<TextInputStream>(sharedInputStream());
    return util::CoTask<StreamReadResult<text::String>>::run(
        [self = std::move(self), maximum]() -> StreamReadResult<text::String> { return self->read(maximum); });
}

auto TextInputStream::coReadLine() -> util::CoTask<StreamReadResult<text::String>> {
    return coReadLine(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadLine(const unit::CpLength maximum) -> util::CoTask<StreamReadResult<text::String>> {
    auto self = std::static_pointer_cast<TextInputStream>(sharedInputStream());
    return util::CoTask<StreamReadResult<text::String>>::run(
        [self = std::move(self), maximum]() -> StreamReadResult<text::String> { return self->readLine(maximum); });
}

auto TextInputStream::coReadAll() -> util::CoTask<StreamReadResult<text::String>> {
    return coReadAll(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadAll(const unit::CpLength maximum) -> util::CoTask<StreamReadResult<text::String>> {
    auto self = std::static_pointer_cast<TextInputStream>(sharedInputStream());
    return util::CoTask<StreamReadResult<text::String>>::run(
        [self = std::move(self), maximum]() -> StreamReadResult<text::String> { return self->readAll(maximum); });
}

auto TextInputStream::coReadBlocks() -> util::CoAsyncGenerator<StreamReadResult<text::String>> {
    return coReadBlocks(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadBlocks(const unit::CpLength maximum)
    -> util::CoAsyncGenerator<StreamReadResult<text::String>> {
    if (maximum.isInfinite() || maximum.isZero()) {
        throw err::ParameterError{"The coroutine text-block length must be positive and finite.", "maximum"};
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

auto TextInputStream::coReadLines() -> util::CoAsyncGenerator<StreamReadResult<text::String>> {
    return coReadLines(cDefaultTextReadMaximum);
}

auto TextInputStream::coReadLines(const unit::CpLength maximum)
    -> util::CoAsyncGenerator<StreamReadResult<text::String>> {
    if (maximum.isInfinite() || maximum.isZero()) {
        throw err::ParameterError{"The coroutine line length must be positive and finite.", "maximum"};
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
