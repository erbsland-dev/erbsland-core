// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProtectedByteBlock.hpp"

#include "../impl/protected_data/ProtectedDataAccess.hpp"
#include "../impl/SecureEraseGuard.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

ProtectedByteBlock::ProtectedByteBlock(const mem::ConstByteSpan data) : _byteLength{data.size()} {
    if (data.empty()) {
        return;
    }
    // The provider receives a borrowed plaintext view and returns only its opaque authenticated ciphertext envelope.
    _envelope = impl::ProtectedDataAccess::access().protect(data, _byteLength);
    _envelope.markAsSensitive();
}

ProtectedByteBlock::ProtectedByteBlock(const mem::ByteBlock &data) : ProtectedByteBlock{data.span()} {
}

ProtectedByteBlock::ProtectedByteBlock(ProtectedByteBlock &&other) noexcept :
    _envelope{std::move(other._envelope)}, _byteLength{other._byteLength} {
    other._byteLength = unit::ByteLength::zero();
}

auto ProtectedByteBlock::operator=(ProtectedByteBlock &&other) noexcept -> ProtectedByteBlock & {
    if (this != &other) {
        secureErase();
        _envelope = std::move(other._envelope);
        _byteLength = other._byteLength;
        other._byteLength = unit::ByteLength::zero();
    }
    return *this;
}

void ProtectedByteBlock::secureErase() noexcept {
    _envelope.secureErase();
    _envelope = {};
    _byteLength = unit::ByteLength::zero();
}

auto ProtectedByteBlock::unprotect() const -> mem::ByteBlock {
    if (isEmpty()) {
        return {};
    }
    // Provider contracts return authenticated plaintext in sensitive storage; ownership transfers to the caller.
    return impl::ProtectedDataAccess::access().unprotect(_envelope.span(), _byteLength);
}

void ProtectedByteBlock::withUnprotectedData(const UnprotectedDataFn &callback) const {
    if (!callback) {
        throw err::ParameterError{"A protected-data callback is required."_el, "callback"_el};
    }
    // Keep the authenticated plaintext in sensitive owning storage while the callback borrows its view. The guard
    // erases that storage after callback success and while unwinding a callback exception.
    auto plaintext = unprotect();
    const auto eraseGuard = impl::SecureEraseGuard{plaintext};
    callback(plaintext.span());
}

}
