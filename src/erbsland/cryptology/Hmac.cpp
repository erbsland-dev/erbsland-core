// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Hmac.hpp"

#include "impl/authentication/HmacWorker.hpp"
#include "impl/authentication/HmacWorkerFactory.hpp"

#include "../err/LogicError.hpp"
#include "../mem/ByteBlock.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

Hmac::Hmac() noexcept = default;

Hmac::Hmac(const HashAlgorithm algorithm, mem::ByteBlock key) {
    // The facade transfers the caller's shared key allocation into the library's sensitive-memory lifecycle.
    // createHmacWorker() synchronously normalizes the key according to RFC 2104 section 2 before this copy is released.
    key.markAsSensitive();
    _worker = impl::createHmacWorker(algorithm, key.span());
}

Hmac::Hmac(const HashAlgorithm algorithm, const mem::ConstByteSpan key) :
    Hmac{algorithm, mem::ByteBlock::fromSpan(key)} {
}

Hmac::~Hmac() {
    secureErase();
}

Hmac::Hmac(Hmac &&other) noexcept : _worker{std::move(other._worker)} {
}

auto Hmac::operator=(Hmac &&other) noexcept -> Hmac & {
    if (this != &other) {
        secureErase();
        _worker = std::move(other._worker);
    }
    return *this;
}

void Hmac::reset() {
    requireValid();
    _worker->reset();
}

void Hmac::secureErase() noexcept {
    if (_worker != nullptr) {
        // The worker erases its key-dependent hash states before its storage is released.
        _worker->secureErase();
        _worker.reset();
    }
}

void Hmac::update(const mem::ConstByteSpan data) {
    requireValid();
    _worker->update(data);
}

void Hmac::update(const mem::ByteBlock &data) {
    update(data.span());
}

void Hmac::update(const text::String &text) {
    const auto bytes = text::impl::UnsafeU8StringAccess{text}.dataView().dataSpan();
    update(mem::toConstByteSpan(bytes));
}

auto Hmac::finalize() -> mem::ByteBlock {
    requireValid();
    return _worker->finalize();
}

auto Hmac::verify(const mem::ConstByteSpan expected) -> bool {
    requireValid();
    // This API does not implement RFC 2104 tag truncation. Reject a non-HashLen tag before finalizing the stream.
    if (expected.size() != _worker->algorithm().digestSize().toSizeT()) {
        return false;
    }
    // Equal-length tags are compared by inspecting every byte without content-dependent short-circuiting.
    return finalize().isEqualConstTime(expected);
}

auto Hmac::verify(const mem::ByteBlock &expected) -> bool {
    return verify(expected.span());
}

auto Hmac::algorithm() const -> HashAlgorithm {
    requireValid();
    return _worker->algorithm();
}

void Hmac::requireValid() const {
    if (!isValid()) {
        throw err::LogicError{"Cannot use an invalid HMAC state."_el};
    }
}

}
