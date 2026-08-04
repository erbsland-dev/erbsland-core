// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cryptology/x509/X509Certificate.hpp>
#include <erbsland/cryptology/x509/X509CertificateProfileMode.hpp>
#include <erbsland/mem/ByteBlock.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    const auto bytes = erbsland::mem::ByteBlock::fromVector(std::vector<uint8_t>{data, data + size});
    static_cast<void>(erbsland::cryptology::X509Certificate::fromDer(
        bytes, erbsland::cryptology::X509CertificateProfileMode::Strict));
    static_cast<void>(erbsland::cryptology::X509Certificate::fromDer(
        bytes, erbsland::cryptology::X509CertificateProfileMode::Compatible));
    return 0;
}
