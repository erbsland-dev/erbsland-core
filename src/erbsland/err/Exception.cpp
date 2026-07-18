// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Exception.hpp"

#include "impl/ExceptionDiagnostic.hpp"

#include "../text/impl/UnsafeU8StringAccess.hpp"
#include "../text/StringEditor.hpp"

#include <utility>

namespace erbsland::err {

Exception::Exception(const std::string_view reason) noexcept : Exception{text::String{reason}} {
}

Exception::Exception(const std::string_view reason, std::exception_ptr cause) noexcept :
    Exception{text::String{reason}, std::move(cause)} {
}

auto Exception::what() const noexcept -> mem::UnsafeConstCharPtr {
    if (_reason.isEmpty()) {
        const static std::string staticEmpty{};
        return staticEmpty.data();
    }
    return text::impl::UnsafeU8StringAccess{_reason}.dataView().dataSpan().data();
}

auto Exception::toString() const noexcept -> text::String {
    // For the default implementation, we only have the reason text to return.
    return reason();
}

auto Exception::diagnostic() const -> DiagnosticConstPtr {
    return std::make_shared<impl::ExceptionDiagnostic>(toString());
}

}
