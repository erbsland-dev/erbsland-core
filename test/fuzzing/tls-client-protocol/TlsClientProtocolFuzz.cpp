// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordContentType.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/network/Host.hpp>
#include <erbsland/network/impl/TlsClientProtocol.hpp>
#include <erbsland/network/impl/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/TlsWireWriter.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unit/ByteLength.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

class TlsClientProtocolFuzzInput final {
private:
    using Protocol = erbsland::network::impl::TlsClientProtocol;
    using TestAccess = erbsland::network::impl::TlsClientProtocolTestAccess;
    using Writer = erbsland::network::impl::TlsWireWriter;

public:
    explicit TlsClientProtocolFuzzInput(const std::span<const uint8_t> data) : _data{data} {}

    [[nodiscard]] auto run() const -> int {
        static constexpr auto cMaximumInputLength = std::size_t{64U * 1024U};
        if (_data.empty() || _data.size() > cMaximumInputLength) {
            return 0;
        }

        auto protocol = createProtocol();
        auto access = TestAccess{protocol};
        start(protocol, access);
        const auto mode = _data.front() % 5U;
        const auto payload = _data.subspan(1U);
        if (mode == 0U) {
            feedFragmented(protocol, erbsland::mem::toConstByteSpan(payload), _data.front());
            return 0;
        }

        establish(protocol, access);
        if (mode == 4U) {
            feedFragmented(protocol, erbsland::mem::toConstByteSpan(payload), _data.front());
            return 0;
        }

        auto serverSender = access.serverApplicationEncryptor();
        const auto contentType = mode == 1U ? erbsland::cryptology::TlsRecordContentType::Handshake
            : mode == 2U                    ? erbsland::cryptology::TlsRecordContentType::Alert
                                            : erbsland::cryptology::TlsRecordContentType::ApplicationData;
        auto offset = std::size_t{0U};
        do {
            const auto length = std::min<std::size_t>(payload.size() - offset, 1U << 14U);
            const auto content = erbsland::mem::toConstByteSpan(payload.subspan(offset, length));
            const auto record = serverSender.protect(contentType, content);
            feedFragmented(protocol, record.span(), _data.front());
            offset += length;
        } while (
            offset < payload.size() && protocol.state() != erbsland::network::impl::TlsClientProtocolState::Failed);
        return 0;
    }

private:
    [[nodiscard]] static auto createProtocol() -> Protocol {
        using namespace erbsland::text::literals;
        return Protocol{erbsland::network::impl::TlsClientProtocolOptions{
            erbsland::network::Host::fromStringOrThrow("fuzz.example"_el),
            erbsland::cryptology::X509ServerCertificatePolicy{erbsland::cryptology::X509CertificateBundle{}},
            erbsland::time::DateTime::now()}};
    }

    [[nodiscard]] static auto privateKey(const uint8_t value) -> erbsland::cryptology::KeyAgreementPrivateKey {
        return erbsland::cryptology::KeyAgreementPrivateKey::fromBytes(
            erbsland::cryptology::KeyAgreementAlgorithm::X25519,
            erbsland::mem::ByteBlock{erbsland::unit::ByteLength{32U}, value}.span());
    }

    static void start(Protocol &protocol, TestAccess &access) {
        access.start(
            erbsland::mem::ByteBlock{erbsland::unit::ByteLength{32U}, 0x11U},
            erbsland::mem::ByteBlock{erbsland::unit::ByteLength{32U}, 0x22U},
            privateKey(0x44U));
        [[maybe_unused]] const auto clientHelloRecord = protocol.takeTransportOutput();
    }

    static void establish(Protocol &protocol, TestAccess &access) {
        auto serverPrivate = privateKey(0x55U);
        protocol.feedTransport(serverHello(serverPrivate).span());
        auto serverHandshakeSender = access.serverHandshakeEncryptor();

        const auto encryptedExtensions = erbsland::mem::ByteBlock({8U, 0U, 0U, 2U, 0U, 0U});
        protocol.feedTransport(serverHandshakeSender
                .protect(erbsland::cryptology::TlsRecordContentType::Handshake, encryptedExtensions.span())
                .span());
        protocol.resume();
        const auto certificate = erbsland::mem::ByteBlock({11U, 0U, 0U, 4U, 0U, 0U, 0U, 0U});
        const auto certificateVerify = erbsland::mem::ByteBlock({15U, 0U, 0U, 4U, 8U, 4U, 0U, 0U});
        access.acceptServerAuthentication(certificate.span(), certificateVerify.span());
        protocol.resume();
        const auto finished = access.serverFinishedMessage();
        protocol.feedTransport(
            serverHandshakeSender.protect(erbsland::cryptology::TlsRecordContentType::Handshake, finished.span())
                .span());
        protocol.resume();
        while (protocol.takeTransportOutput().has_value()) {}
    }

    [[nodiscard]] static auto serverHello(const erbsland::cryptology::KeyAgreementPrivateKey &serverPrivate)
        -> erbsland::mem::ByteBlock {
        auto extensions = Writer{};
        extensions.writeU16(43U);
        extensions.writeVector16(erbsland::mem::ByteBlock({3U, 4U}).span());
        auto keyShare = Writer{};
        keyShare.writeU16(0x001dU);
        keyShare.writeVector16(serverPrivate.publicKey().span());
        const auto keyShareBytes = keyShare.finish();
        extensions.writeU16(51U);
        extensions.writeVector16(keyShareBytes.span());
        const auto extensionBytes = extensions.finish();

        auto body = Writer{};
        body.writeU16(0x0303U);
        body.writeBytes(erbsland::mem::ByteBlock{erbsland::unit::ByteLength{32U}, 0x33U}.span());
        body.writeVector8(erbsland::mem::ByteBlock{erbsland::unit::ByteLength{32U}, 0x22U}.span());
        body.writeU16(0x1301U);
        body.writeU8(0U);
        body.writeVector16(extensionBytes.span());
        const auto bodyBytes = body.finish();

        auto handshake = Writer{};
        handshake.writeU8(2U);
        handshake.writeU24(static_cast<uint32_t>(bodyBytes.length().toSizeT()));
        handshake.writeBytes(bodyBytes.span());
        const auto handshakeBytes = handshake.finish();
        auto record = Writer{};
        record.writeU8(22U);
        record.writeU16(0x0303U);
        record.writeVector16(handshakeBytes.span());
        return record.finish();
    }

    static void feedFragmented(Protocol &protocol, const erbsland::mem::ConstByteSpan data, const uint8_t selector) {
        const auto fragmentLength = std::size_t{1U + selector % 64U};
        auto offset = std::size_t{0U};
        while (offset < data.size()) {
            const auto length = std::min(fragmentLength, data.size() - offset);
            protocol.feedTransport(data.subspan(offset, length));
            while (protocol.hasCheckpoint()) {
                protocol.resume();
            }
            offset += length;
        }
    }

private:
    std::span<const uint8_t> _data; ///< Mode/fragment selector followed by arbitrary transport or inner content bytes.
};

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    static const auto application = erbsland::core::Application{};
    static_cast<void>(application);
    return TlsClientProtocolFuzzInput{std::span<const uint8_t>{data, size}}.run();
}
