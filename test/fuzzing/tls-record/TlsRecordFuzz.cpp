// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordDecryptor.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordError.hpp>
#include <erbsland/cryptology/tls_record/TlsTrafficSecret.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/unit/ByteLength.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

class TlsRecordFuzzInput final {
public:
    explicit TlsRecordFuzzInput(const std::span<const uint8_t> data) : _data{data} {}

    [[nodiscard]] auto run() const -> int {
        static constexpr auto maximumInputSize = std::size_t{16'645U};
        if (_data.empty() || _data.size() > maximumInputSize) {
            return 0;
        }

        const auto suite = suiteFromSelector(_data.front());
        auto secretBytes = erbsland::mem::ByteBlock{suite.hashAlgorithm().digestSize(), 0x5aU};
        try {
            auto secret = erbsland::cryptology::TlsTrafficSecret::fromBytes(
                suite.hashAlgorithm(), std::move(secretBytes));
            auto decryptor = erbsland::cryptology::TlsRecordDecryptor{suite, std::move(secret)};
            const auto record = erbsland::mem::toConstByteSpan(_data.subspan(1U));
            static_cast<void>(decryptor.unprotect(record));
        } catch (const erbsland::err::Exception &) {
            // Arbitrary framing, authentication, type, padding, bounds, and backend failures are expected inputs.
        }
        return 0;
    }

private:
    [[nodiscard]] static auto suiteFromSelector(const uint8_t selector) noexcept
        -> erbsland::cryptology::TlsCipherSuite {
        switch (selector % 3U) {
        case 0U:
            return erbsland::cryptology::TlsCipherSuite::Aes128GcmSha256;
        case 1U:
            return erbsland::cryptology::TlsCipherSuite::Aes256GcmSha384;
        default:
            return erbsland::cryptology::TlsCipherSuite::ChaCha20Poly1305Sha256;
        }
    }

private:
    std::span<const uint8_t> _data; ///< Suite selector followed by one exact record candidate.
};

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    static const auto application = erbsland::core::Application{};
    static_cast<void>(application);
    return TlsRecordFuzzInput{std::span<const uint8_t>{data, size}}.run();
}
