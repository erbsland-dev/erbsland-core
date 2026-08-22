// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockInputStream.hpp"

#include "../err/OverflowError.hpp"
#include "../err/ParameterError.hpp"
#include "../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::stream {

using namespace text::literals;

ByteBlockInputStream::ByteBlockInputStream(mem::ByteBlock data, InputStreamSettings settings) :
    _data{std::move(data)}, _settings{std::move(settings)} {
    if (_data.isSensitive()) {
        _settings.setSensitive(true);
    }
}

auto ByteBlockInputStream::inputSettings() const noexcept -> const InputStreamSettings & {
    return _settings;
}

auto ByteBlockInputStream::state() const noexcept -> StreamState {
    const auto lock = std::scoped_lock{_mutex};
    return _state;
}

auto ByteBlockInputStream::isReady() const noexcept -> bool {
    return true;
}

auto ByteBlockInputStream::waitForReady() -> StreamWaitStatus {
    return StreamWaitStatus::Ready;
}

auto ByteBlockInputStream::close() -> StreamCloseStatus {
    abort();
    return StreamCloseStatus::Closed;
}

void ByteBlockInputStream::abort() noexcept {
    discardRetainedInput();
    const auto lock = std::scoped_lock{_mutex};
    _state = StreamState::Closed;
}

auto ByteBlockInputStream::readFromSource(const mem::ByteSpan destination, [[maybe_unused]] const ReadDeadline deadline)
    -> StreamReadResult<unit::ByteLength> {
    const auto lock = std::scoped_lock{_mutex};
    verifyOpen();
    const auto remaining =
        _position < _data.endIndex() ? _data.length() - unit::ByteLength{_position.toRawValue()} : unit::ByteLength{};
    const auto length = std::min(unit::ByteLength::fromSizeT(destination.size()), remaining);
    if (length.isZero()) {
        return {StreamReadStatus::Finished, unit::ByteLength{}};
    }
    const auto source = mem::impl::UnsafeByteBlockAccess{_data}.dataView().dataSpan();
    std::copy_n(source.data() + _position.toSizeT(), length.toSizeT(), destination.data());
    _position += length;
    return {StreamReadStatus::Data, length};
}

auto ByteBlockInputStream::sourceSupportsPositioning() const noexcept -> bool {
    return true;
}

auto ByteBlockInputStream::sourcePosition() const -> unit::ByteIndex {
    const auto lock = std::scoped_lock{_mutex};
    verifyOpen();
    return _position;
}

auto ByteBlockInputStream::setSourcePosition(const unit::ByteIndex position) -> StreamPositionStatus {
    if (!position.isValid()) {
        throw err::ParameterError{"A byte-block stream position must be valid."_el, "position"_el};
    }
    const auto lock = std::scoped_lock{_mutex};
    verifyOpen();
    _position = position;
    return StreamPositionStatus::Success;
}

auto ByteBlockInputStream::moveSourcePosition(const StreamPositionOrigin origin, const unit::ByteOffset offset)
    -> StreamPositionStatus {
    const auto lock = std::scoped_lock{_mutex};
    verifyOpen();
    const auto base = origin == StreamPositionOrigin::Start
        ? unit::ByteIndex{}
        : (origin == StreamPositionOrigin::Current ? _position : _data.endIndex());
    try {
        _position = base.movedOrThrow(offset);
    } catch (const err::OverflowError &) {
        throw err::ParameterError{"A byte-block stream position must remain within index bounds."_el, "offset"_el};
    }
    return StreamPositionStatus::Success;
}

void ByteBlockInputStream::verifyOpen() const {
    if (_state != StreamState::Open) {
        throwError("Failed to read from the byte-block stream."_el, "The byte-block input stream is closed."_el);
    }
}

}
