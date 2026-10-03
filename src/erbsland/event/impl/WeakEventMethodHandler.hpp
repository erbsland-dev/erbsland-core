// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BindEventTraits.hpp"

#include <memory>

namespace erbsland::event::impl {

/// Lock a weak recipient only while dispatching its member function.
/// @tparam tRecipient The recipient type, possibly const or derived from the method's class.
/// @tparam tMethod The member-function pointer type.
/// @tested{BindWeakEventTest}
template <typename tRecipient, typename tMethod>
class WeakEventMethodHandler final {
public:
    /// Store a weak recipient and its method.
    /// @param recipient The recipient; empty and expired pointers are allowed.
    /// @param method The non-null member-function pointer.
    WeakEventMethodHandler(std::weak_ptr<tRecipient> recipient, tMethod method);

public:
    /// Invoke the method if the recipient still exists at dispatch time.
    /// @tparam tArguments The forwarded argument types.
    /// @param arguments The arguments to forward to the method.
    /// @throws Any exception thrown by the method.
    template <typename... tArguments>
        requires EventVoidInvocable<tMethod &, tRecipient &, tArguments...>
    void operator()(tArguments &&...arguments);

private:
    std::weak_ptr<tRecipient> _recipient; ///< Recipient checked only during delivery.
    tMethod _method;                      ///< Member function to invoke on the target loop.
};

}

#include "WeakEventMethodHandler.tpp"
