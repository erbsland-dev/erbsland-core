// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadSecret.hpp"

#include "../Terminal.hpp"

#include "../../err/ParameterError.hpp"
#include "../../mem/impl/SecureErase.hpp"
#include "../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../text/u8/impl/U8Encoding.hpp"
#include "../../text/u8/impl/U8Writer.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace erbsland::cterm::impl {

using unit::CpIndex;
using unit::CpLength;

ReadSecret::ReadSecret(TerminalPtr terminal, ReadLineOptions options) :
    ReadSecret{std::move(terminal), prepareOptions(std::move(options))} {
}

ReadSecret::ReadSecret(TerminalPtr terminal, PreparedOptions prepared) :
    ReadLineBase{std::move(terminal), std::move(prepared.options)}, _maximumLength{prepared.maximumLength} {
}

ReadSecret::~ReadSecret() {
    stop();
    discardText();
    _committed = {};
}

auto ReadSecret::prepareOptions(ReadLineOptions options) -> PreparedOptions {
    if (!options.history().isEmpty()) {
        throw err::ParameterError{"Secret input does not support history.", "history"};
    }
    if (!options.currentText().isEmpty()) {
        throw err::ParameterError{"Secret input does not support initial text.", "currentText"};
    }
    const auto maximumLength = std::min(options.maximumLength(), cterm::ReadSecret::cMaximumLength);
    options.setMaximumLength(maximumLength);
    options.setMaximumLines(unit::LineCount{1U});
    options.setMaximumDisplayLines(unit::LineCount{1U});
    return {std::move(options), maximumLength};
}

void ReadSecret::start() {
    purgePendingInput();
    try {
        startBase();
    } catch (...) {
        discardText();
        _committed = {};
        purgePendingInput();
        throw;
    }
}

auto ReadSecret::update() -> ReadLineResult {
    try {
        return result(updateBase());
    } catch (...) {
        discardText();
        purgePendingInput();
        throw;
    }
}

auto ReadSecret::waitForInput() -> ReadLineResult {
    purgePendingInput();
    try {
        auto output = result(waitForInputBase());
        purgePendingInput();
        _committed = {};
        return output;
    } catch (...) {
        discardText();
        _committed = {};
        purgePendingInput();
        throw;
    }
}

void ReadSecret::stop() noexcept {
    stopBase();
    discardText();
    _committed = {};
    purgePendingInput();
}

void ReadSecret::resetText() {
    discardText();
    _committed = {};
}

void ReadSecret::discardText() noexcept {
    mem::impl::secureErase(std::as_writable_bytes(std::span{_characters}));
    _length = 0U;
    _mask = {};
}

void ReadSecret::commitText() {
    auto byteLength = unit::ByteLength{};
    for (auto index = std::size_t{}; index < _length; ++index) {
        byteLength += text::impl::utf8::encodedLength(_characters[index]);
    }
    try {
        auto buffer = text::impl::UnsafeU8StringBuffer{byteLength, true};
        auto writer = text::impl::U8Writer{std::span<char>{buffer.data(), buffer.capacity().toSizeT()}};
        for (auto index = std::size_t{}; index < _length; ++index) {
            writer.write(_characters[index]);
        }
        _committed = buffer.take(unit::ByteLength::fromSizeT(writer.position()));
        discardText();
    } catch (...) {
        discardText();
        throw;
    }
}

auto ReadSecret::insertKeyText(const Key &key, const CpIndex index) -> CpLength {
    if (!key.modifiers().empty()) {
        return {};
    }
    auto characters = text::CombinedChar::Storage{};
    auto count = std::size_t{};
    if (key.type() == Key::Space) {
        characters[0] = U' ';
        count = 1U;
    } else if (key.type() == Key::Character || key.type() == Key::Combined) {
        characters = key.combinedCharacter().characters();
        count = key.combinedCharacter().characterCount().toSizeT();
    } else {
        return {};
    }
    if (count == 0U || _length + count > _maximumLength.toSizeT() || index.toSizeT() > _length) {
        return {};
    }
    for (auto offset = std::size_t{}; offset < count; ++offset) {
        if (characters[offset].isNull() || characters[offset].isControl()) {
            return {};
        }
    }
    const auto position = index.toSizeT();
    std::move_backward(
        _characters.begin() + static_cast<std::ptrdiff_t>(position),
        _characters.begin() + static_cast<std::ptrdiff_t>(_length),
        _characters.begin() + static_cast<std::ptrdiff_t>(_length + count));
    std::copy_n(characters.begin(), count, _characters.begin() + static_cast<std::ptrdiff_t>(position));
    _length += count;
    auto bullets = text::U32StringEditor{};
    bullets.append(text::Char{U'\u2022'}, CpLength::fromSizeT(count));
    _mask.insert(index, bullets);
    return CpLength::fromSizeT(count);
}

auto ReadSecret::insertNewLine([[maybe_unused]] const CpIndex index) -> bool {
    return false;
}

void ReadSecret::eraseText(const CpIndex index, const CpLength length) noexcept {
    if (index.toSizeT() >= _length || length.isZero()) {
        return;
    }
    const auto count = std::min(length.toSizeT(), _length - index.toSizeT());
    const auto source = index.toSizeT() + count;
    std::move(
        _characters.begin() + static_cast<std::ptrdiff_t>(source),
        _characters.begin() + static_cast<std::ptrdiff_t>(_length),
        _characters.begin() + static_cast<std::ptrdiff_t>(index.toSizeT()));
    eraseCharacters(_length - count, count);
    _length -= count;
    _mask.remove(unit::CpRange{index, CpLength::fromSizeT(count)});
}

auto ReadSecret::previousUnitStart(CpIndex index) const noexcept -> CpIndex {
    if (index.isZero()) {
        return {};
    }
    --index;
    while (!index.isZero() && _characters[index.toSizeT()].displayWidth() == 0) {
        --index;
    }
    return index;
}

auto ReadSecret::nextUnitEnd(CpIndex index) const noexcept -> CpIndex {
    const auto end = CpIndex::fromSizeT(_length);
    if (index >= end) {
        return end;
    }
    ++index;
    while (index < end && _characters[index.toSizeT()].displayWidth() == 0) {
        ++index;
    }
    return index;
}

void ReadSecret::eraseCharacters(const std::size_t begin, const std::size_t count) noexcept {
    mem::impl::secureErase(std::as_writable_bytes(std::span{_characters}.subspan(begin, count)));
}

auto ReadSecret::result(const ReadLineStatus status) const -> ReadLineResult {
    return status == ReadLineStatus::Committed ? ReadLineResult{status, _committed} : ReadLineResult{status, {}};
}

void ReadSecret::purgePendingInput() noexcept {
    try {
        terminal()->input().purgePendingInput();
    } catch (...) {}
}

}
