// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Hasher.hpp"

#include "impl/HashWorker.hpp"
#include "impl/HashWorkerFactory.hpp"

#include "../err/LogicError.hpp"
#include "../mem/ByteBlock.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"

#include <utility>

namespace erbsland::cryptology {

Hasher::Hasher(const HashAlgorithm algorithm) : _worker{impl::createHashWorker(algorithm)} {
}

Hasher::~Hasher() = default;
Hasher::Hasher(const Hasher &) = default;
Hasher::Hasher(Hasher &&) noexcept = default;
auto Hasher::operator=(const Hasher &) -> Hasher & = default;
auto Hasher::operator=(Hasher &&) noexcept -> Hasher & = default;

auto Hasher::isValid() const noexcept -> bool {
    return _worker != nullptr;
}

auto Hasher::algorithm() const -> HashAlgorithm {
    requireValid();
    return _worker->algorithm();
}

void Hasher::reset() {
    workerForWrite().reset();
}

void Hasher::update(const std::span<const std::byte> data) {
    workerForWrite().update(data);
}

void Hasher::update(const std::span<const mem::Byte> data) {
    update(std::as_bytes(data));
}

void Hasher::update(const mem::ByteBlock &data) {
    update(data.bytes());
}

void Hasher::update(const text::String &text) {
    const auto bytes = text::impl::UnsafeU8StringAccess{text}.dataView().dataSpan();
    update(std::as_bytes(bytes));
}

auto Hasher::finalize() -> mem::ByteBlock {
    return workerForWrite().finalize();
}

auto Hasher::workerForWrite() -> impl::HashWorker & {
    requireValid();
    if (_worker.use_count() > 1) {
        _worker = _worker->clone();
    }
    return *_worker;
}

void Hasher::requireValid() const {
    if (!isValid()) {
        throw err::LogicError{"Cannot use an invalid hasher."};
    }
}

}
