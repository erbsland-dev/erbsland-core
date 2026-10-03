// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BindEventTraits.hpp"

#include <memory>

namespace erbsland::event::impl {

/// Retain a recipient for repeated member-function invocations.
/// @tparam tRecipient The recipient type, possibly const or derived from the method's class.
/// @tparam tMethod The member-function pointer type.
/// @tested{BindEventTest}
template <typename tRecipient, typename tMethod>
class EventMethodHandler final {
public:
    /// Retain a recipient and its method.
    /// @param recipient The non-null recipient.
    /// @param method The non-null member-function pointer.
    EventMethodHandler(std::shared_ptr<tRecipient> recipient, tMethod method);

public:
    /// Invoke the retained recipient's method.
    /// @tparam tArguments The forwarded argument types.
    /// @param arguments The arguments to forward to the method.
    /// @throws Any exception thrown by the method.
    template <typename... tArguments>
        requires EventVoidInvocable<tMethod &, tRecipient &, tArguments...>
    void operator()(tArguments &&...arguments);

private:
    std::shared_ptr<tRecipient> _recipient; ///< Recipient retained through pending invocations.
    tMethod _method;                        ///< Member function to invoke on the target loop.
};

}

#include "EventMethodHandler.tpp"
