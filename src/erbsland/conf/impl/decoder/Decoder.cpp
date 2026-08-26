// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Decoder.hpp"

#include "Transaction.hpp"

#include <cassert>

namespace erbsland::conf::impl {

auto Decoder::advanceWhile(const text::AsciiCategory category, unit::CpLength maximum) -> unit::CpLength {
    auto count = unit::CpLength::zero();
    while (character().isAsciiCategory(category) && (maximum.isInfinite() || count < maximum)) {
        next();
        ++count;
    }
    return count;
}

auto Decoder::startTransaction(Transaction &transaction) noexcept -> DecoderState {
    transaction._parent = _activeTransaction;
    _activeTransaction = std::ref(transaction);
    return decoderState();
}

void Decoder::commitTransaction(Transaction &transaction) noexcept {
    assert(_activeTransaction.has_value() && &_activeTransaction->get() == &transaction);
    assert(transaction.state() == Transaction::State::Committed);
    _activeTransaction = transaction._parent;
    transaction._parent.reset();
}

void Decoder::rollbackTransaction(Transaction &transaction) noexcept {
    assert(_activeTransaction.has_value() && &_activeTransaction->get() == &transaction);
    assert(transaction.state() == Transaction::State::RolledBack);
    restoreDecoderState(transaction._checkpoint);
    _activeTransaction = transaction._parent;
    transaction._parent.reset();
}

auto Decoder::transactionCapturedSize(const Transaction &transaction) const noexcept -> std::size_t {
    assert(transaction.state() == Transaction::State::Open);
    assert(_activeTransaction.has_value() && &_activeTransaction->get() == &transaction);
    const auto current = decoderState().location();
    assert(current.line() == transaction._checkpoint.location().line());
    return current.column().toSizeT() - transaction._checkpoint.location().column().toSizeT();
}

auto Decoder::captureTransactionContent(const Transaction &transaction) const noexcept -> text::String {
    assert(transaction.state() == Transaction::State::Open);
    assert(_activeTransaction.has_value() && &_activeTransaction->get() == &transaction);
    return captureFromDecoderState(transaction._checkpoint);
}

}
