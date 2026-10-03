// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessState.hpp"

#include "../../../mem/impl/UnsafeRingBufferAccess.hpp"
#include "../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../text/impl/UnsafeU8StringEditorAccess.hpp"
#include "../../../text/StringBomMode.hpp"
#include "../../../text/StringDecodeBuffer.hpp"
#include "../../../text/StringEncoding.hpp"

#include <algorithm>

namespace erbsland::system::impl {

using namespace mem;
using namespace text;
using namespace unit;
using namespace text::literals;

void SubprocessState::receive(const bool error, const std::string_view bytes) {
    storeOutput(error, bytes);
    wake();
}

void SubprocessState::storeOutput(const bool error, const std::string_view bytes) {
    const auto lock = std::scoped_lock{_mutex};
    auto &output = error ? _errorOutput : _output;
    storeOutput(output, removeOutputBom(output, toConstByteSpan(std::span<const char>{bytes.data(), bytes.size()})));
}

auto SubprocessState::removeOutputBom(Output &output, ConstByteSpan bytes) -> ConstByteSpan {
    if (output.bomResolved || !_options.removesOutputBom()) {
        output.bomResolved = true;
        return bytes;
    }
    const auto bom = StringEncoding{StringEncoding::Utf8}.bomBytes(StringBomMode::Require);
    while (!bytes.empty()) {
        if (bytes.front() != bom[output.bomPrefixLength]) {
            flushOutputBomPrefix(output);
            break;
        }
        bytes = bytes.subspan(1);
        ++output.bomPrefixLength;
        if (output.bomPrefixLength == bom.size()) {
            output.bomPrefixLength = 0;
            output.bomResolved = true;
            break;
        }
    }
    return bytes;
}

void SubprocessState::flushOutputBomPrefix(Output &output) {
    if (output.bomResolved) {
        return;
    }
    const auto bom = StringEncoding{StringEncoding::Utf8}.bomBytes(StringBomMode::Require);
    storeOutput(output, bom.first(output.bomPrefixLength));
    output.bomPrefixLength = 0;
    output.bomResolved = true;
}

void SubprocessState::storeOutput(Output &output, const ConstByteSpan bytes) {
    if (bytes.empty()) {
        return;
    }
    auto count = std::size_t{};
    if (output.handler) {
        count = output.pending->write(bytes).toSizeT();
    } else {
        const auto available = _options.captureLimit() - output.captured.length();
        count = std::min(available.toSizeT(), bytes.size());
        text::impl::UnsafeU8StringEditorAccess::appendTools(output.captured).append(bytes.first(count));
    }
    if (count < bytes.size()) {
        output.truncated = true;
        if (output.handler) {
            output.gap = true;
            if (_overflow == Overflow::None) {
                _overflow = Overflow::Pending;
            }
        }
    }
}

auto SubprocessState::chunkLength(const String &bytes, const ByteCount maximum, const bool finished) -> ByteCount {
    const auto count = std::min(bytes.length(), maximum);
    if (finished && count == bytes.length()) {
        return count;
    }
    const auto raw = text::impl::UnsafeU8StringAccess{bytes}.dataView().dataSpan();
    const auto end = count.toSizeT();
    // Only a trailing valid prefix can need more data; malformed bytes remain unchanged.
    for (auto back = std::size_t{1}; back <= std::min(end, std::size_t{3}); ++back) {
        const auto first = static_cast<std::uint8_t>(raw[end - back]);
        if (first < 0xC2U || first > 0xF4U) {
            continue;
        }
        auto probe = StringDecodeBuffer{ByteCount{4}, StringEncoding::Utf8};
        probe.write(std::string_view{raw.data() + end - back, back});
        if (probe.codePointStatus() == StringDecodeBuffer::CodePointStatus::NeedMoreData) {
            return ByteCount::fromSizeT(end - back);
        }
        break;
    }
    return count;
}

auto SubprocessState::pendingText(Output &output) const -> String {
    auto result = StringEditor{};
    auto remaining = _options.maximumOutputChunkSize();
    auto access = mem::impl::UnsafeRingBufferAccess{*output.pending};
    for (const auto span : access.readableSpans()) {
        const auto count = std::min(remaining.toSizeT(), span.size());
        text::impl::UnsafeU8StringEditorAccess::appendTools(result).append(span.first(count));
        remaining -= ByteCount::fromSizeT(count);
    }
    return result;
}

auto SubprocessState::deliverableLength(const Output &output, const String &pending) const -> ByteCount {
    return chunkLength(
        pending,
        _options.maximumOutputChunkSize(),
        (_phase == Phase::Completed || output.gap) && pending.length() == output.pending->length());
}

auto SubprocessState::outputReady(Output &output) const -> bool {
    return !output.pending->isEmpty() && !deliverableLength(output, pendingText(output)).isZero();
}

auto SubprocessState::takeOutput(const bool error) -> OutputDelivery {
    const auto lock = std::scoped_lock{_mutex};
    auto &output = error ? _errorOutput : _output;
    if (_observation == Observation::Closed || !output.handler || output.pending->isEmpty()) {
        return {};
    }
    const auto pending = pendingText(output);
    const auto count = deliverableLength(output, pending);
    if (count.isZero()) {
        return {};
    }
    auto block = pending.slice(StringSide::Front, count);
    mem::impl::UnsafeRingBufferAccess{*output.pending}.consumeRead(count);
    if (output.pending->isEmpty()) {
        output.gap = false;
    }
    return {output.handler, std::move(block)};
}

void SubprocessState::deliverOutput(const bool error) {
    const auto delivery = takeOutput(error);
    if (delivery.callback) {
        delivery.callback(delivery.block);
    }
}

auto SubprocessState::captured(const bool error) const -> String {
    const auto lock = std::scoped_lock{_mutex};
    return String{(error ? _errorOutput : _output).captured};
}

auto SubprocessState::truncated(const bool error) const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return (error ? _errorOutput : _output).truncated;
}

void SubprocessState::setOutput(const bool error, system::SubprocessEventEditor::OutputFn callback) {
    const auto lock = std::scoped_lock{_mutex};
    auto &output = error ? _errorOutput : _output;
    output.handler = std::move(callback);
    if (!output.handler) {
        output.pending->clear();
        output.gap = false;
    }
}

}
