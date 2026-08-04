// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/String.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    static auto application = erbsland::core::Application{};
    application.cryptologyConfiguration().setProtectedDataMode(erbsland::cryptology::ProtectedDataMode::InternalOnly);
    if (size > 64U * 1024U) {
        return 0;
    }

    // RFC 5208 section 6 and RFC 7468: exercise both strict DER and exact PRIVATE KEY PEM entry points.
    if (size != 0U && data[0U] == static_cast<uint8_t>('-')) {
        const auto text = erbsland::text::String{std::string_view{reinterpret_cast<const char *>(data), size}};
        auto key = erbsland::cryptology::SigningPrivateKey::fromPem(text);
        key.secureErase();
    } else {
        const auto der = erbsland::mem::ByteBlock::fromSpan(std::span<const uint8_t>{data, size});
        auto key = erbsland::cryptology::SigningPrivateKey::fromDer(der);
        key.secureErase();
    }
    return 0;
}
