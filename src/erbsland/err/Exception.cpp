// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Exception.hpp"

#include "impl/ExceptionDiagnostic.hpp"

#include "../text/impl/PlatformU8StringAccess.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"
#include "../text/StringEditor.hpp"

#include <utility>

namespace erbsland::err {

using namespace text;

Exception::Exception(String reason) : _reason{normalizedReason(std::move(reason))} {
}

Exception::Exception(String reason, std::exception_ptr cause) :
    _reason{normalizedReason(std::move(reason))}, _cause{std::move(cause)} {
}

Exception::Exception(const std::string_view reason) : Exception{String{reason}} {
}

Exception::Exception(const std::string_view reason, std::exception_ptr cause) :
    Exception{String{reason}, std::move(cause)} {
}

auto Exception::what() const noexcept -> mem::UnsafeConstCharPtr {
    return text::impl::PlatformU8StringAccess{_reason}.nullTerminatedCharPtr();
}

auto Exception::toString() const noexcept -> String {
    // For the default implementation, we only have the reason text to return.
    return reason();
}

auto Exception::diagnostic() const -> DiagnosticConstPtr {
    return std::make_shared<impl::ExceptionDiagnostic>(toString());
}

auto Exception::normalizedReason(String reason) -> String {
    if (text::impl::UnsafeU8StringAccess{reason}.dataView().isSlice()) {
        return reason.copy();
    }
    return reason;
}

}
