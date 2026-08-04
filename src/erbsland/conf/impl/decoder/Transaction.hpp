// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Decoder_fwd.hpp"
#include "DecoderState.hpp"
#include "Transaction_fwd.hpp"

#include "../utilities/InternalView_fwd.hpp"

#include "../../../text/String.hpp"

#include <functional>
#include <memory>
#include <optional>

namespace erbsland::conf::impl {

/// A transaction scope that allows backtracking.
class Transaction final {
public:
    /// The state of this transaction.
    enum class State : uint8_t {
        Open,       ///< The transaction is open and receiving characters.
        Committed,  ///< The transaction is closed and marked as committed.
        RolledBack, ///< The transaction is closed and marked as rolled back.
    };

public:
    /// Create a new transaction scope for this lexer.
    /// @param decoder The decoder to checkpoint.
    explicit Transaction(Decoder &decoder) noexcept;

    /// When not committed, roll the transaction back on destruction.
    ~Transaction();

    // defaults/deletions
    Transaction(const Transaction &) = delete;
    Transaction(Transaction &&) = delete;
    auto operator=(const Transaction &) -> Transaction & = delete;
    auto operator=(Transaction &&) -> Transaction & = delete;

public: // user functions
    /// Get the number of captured decoded code points.
    /// @return The number of decoded code points between the checkpoint and the current character.
    [[nodiscard]] auto capturedSize() const noexcept -> std::size_t;

    /// Access the captured text as a string.
    [[nodiscard]] auto capturedString() const noexcept -> text::String;

    /// Commit this transaction.
    void commit() noexcept;

    /// Roll the transaction back.
    void rollback() noexcept;

private:
    friend class Decoder;

    /// Get the state of this transaction.
    [[nodiscard]] auto state() const noexcept -> State { return _state; }

public:
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Transaction &object) -> InternalViewPtr;
#endif

private:
    Decoder &_decoder;                                          ///< The decoder owning the active transaction chain.
    DecoderState _checkpoint;                                   ///< The decoder state at the start of the transaction.
    std::optional<std::reference_wrapper<Transaction>> _parent; ///< The active outer transaction, if nested.
    State _state{State::Open};                                  ///< The state of this transaction.
};

}
