// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Registry.hpp"

#include "cases/aes/AesCase.hpp"
#include "cases/ghash/GHashCase.hpp"
#include "cases/stream/StreamCase.hpp"

namespace app::constant_time {

using namespace el::text::literals;

void Registry::addSymmetric() {
    for (const auto size : {std::size_t{16}, std::size_t{32}}) {
        for (const auto operation : {AesOperation::KeyExpansion, AesOperation::Encrypt, AesOperation::Decrypt}) {
            add<AesCase>(size, operation, false);
        }
    }
    add<GHashCase>(false, false);
    add<GHashCase>(true, false);
    for (const auto size : {std::size_t{1}, std::size_t{64}, std::size_t{256}, std::size_t{257}}) {
        for (
            const auto operation :
            {StreamOperation::ChaCha20,
                StreamOperation::Poly1305,
                StreamOperation::AesGcm,
                StreamOperation::ChaCha20Poly1305}) {
            add<StreamCase>(size, operation, false);
        }
    }
}

}
