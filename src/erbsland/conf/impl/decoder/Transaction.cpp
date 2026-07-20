// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Transaction.hpp"

#include "Decoder.hpp"

#include "../utilities/InternalView.hpp"

#include <cassert>

namespace erbsland::conf::impl {

Transaction::Transaction(Decoder &decoder) noexcept : _decoder{decoder} {
    _checkpoint = decoder.startTransaction(*this);
}

Transaction::~Transaction() {
    // If at this point, the transaction is still open, roll it back.
    if (_state == State::Open) {
        _state = State::RolledBack;
        _decoder.rollbackTransaction(*this);
    }
}

auto Transaction::capturedSize() const noexcept -> std::size_t {
    return _decoder.transactionCapturedSize(*this);
}

auto Transaction::capturedString() const noexcept -> text::String {
    return _decoder.captureTransactionContent(*this);
}

void Transaction::commit() noexcept {
    assert(_state == State::Open); // A transaction must be open to be committed
    _state = State::Committed;
    _decoder.commitTransaction(*this);
}

void Transaction::rollback() noexcept {
    assert(_state == State::Open); // A transaction must be open to be rolled back
    _state = State::RolledBack;
    _decoder.rollbackTransaction(*this);
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Transaction &object) -> InternalViewPtr {
    auto result = InternalView::create();
    result->setValue("characterIndex", object._checkpoint.characterIndex().toSizeT());
    result->setValue("nextByteIndex", object._checkpoint.nextByteIndex().toSizeT());
    result->setValue("location", object._checkpoint.location().toString());
    switch (object._state) {
    case Transaction::State::Open:
        result->setValue(u8"state", u8"open");
        break;
    case Transaction::State::Committed:
        result->setValue(u8"state", u8"committed");
        break;
    case Transaction::State::RolledBack:
        result->setValue(u8"state", u8"rolled-back");
        break;
    }
    return result;
}
#endif

}
