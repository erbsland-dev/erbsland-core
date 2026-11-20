// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocolFuzz.hpp"

#include "TlsFuzzFixture.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteSpan.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/KeyAgreementAlgorithm.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolConfiguration.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolOptions.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/Timestamp.hpp>
#include <erbsland/TlsClientAuthentication.hpp>
#include <erbsland/TlsRecordContentType.hpp>
#include <erbsland/TlsRecordEncryptor.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>

auto TlsServerProtocolFuzz::run() const -> int {
    static constexpr auto cMaximumInputLength = std::size_t{64U * 1024U};
    if (_data.empty() || _data.size() > cMaximumInputLength) {
        return 0;
    }

    const auto mode = _data.front() % 7U;
    const auto payload = _data.subspan(1U);
    auto server = createServer(mode >= 5U || _data.front() >= 7U);
    server.start();
    if (mode == 0U) {
        feedFragmented(server, erbsland::mem::toConstByteSpan(payload), _data.front());
        server.abort();
        return 0;
    }

    auto client = createClient(_data.front() >= 7U);
    client.start();
    if (mode >= 5U) {
        static_cast<void>(transferClientOutput(client, server));
        tls_fuzz_fixture::resume(server);
        auto sender = ServerTestAccess{server}.clientHandshakeEncryptor();
        if (mode == 6U) {
            // Parse and validate a real client chain before fuzzing CertificateVerify and later states.
            using Writer = erbsland::network::impl::TlsWireWriter;
            auto entry = Writer{};
            entry.writeVector24(tls_fuzz_fixture::client().certificates.certificates().first().toDer().span());
            entry.writeVector16({});
            auto body = Writer{};
            body.writeVector8({});
            body.writeVector24(entry.finish().span());
            const auto bodyBytes = body.finish();
            auto message = Writer{};
            message.writeU8(11U);
            message.writeU24(static_cast<uint32_t>(bodyBytes.length().toSizeT()));
            message.writeBytes(bodyBytes.span());
            const auto record =
                sender.protect(erbsland::cryptology::TlsRecordContentType::Handshake, message.finish().span());
            server.feedTransport(record.span());
            tls_fuzz_fixture::resume(server);
            if (server.state() == erbsland::network::impl::TlsServerProtocolState::Failed) {
                std::abort();
            }
        }
        if (!payload.empty()) {
            const auto record = sender.protect(
                erbsland::cryptology::TlsRecordContentType::Handshake, erbsland::mem::toConstByteSpan(payload));
            feedFragmented(server, record.span(), _data.front());
            tls_fuzz_fixture::resume(server);
        }
        return 0;
    }
    if (!establish(client, server)) {
        std::abort();
    }
    if (mode == 4U) {
        feedFragmented(server, erbsland::mem::toConstByteSpan(payload), _data.front());
        return 0;
    }

    // RFC 8446 Sections 5 and 7.3: protect fuzzer-controlled TLSInnerPlaintext with the exact generation-zero
    // client application traffic key so authenticated post-handshake parsing is reached without bypassing AEAD.
    auto clientSender = ServerTestAccess{server}.clientApplicationEncryptor();
    const auto contentType = mode == 1U ? erbsland::cryptology::TlsRecordContentType::Handshake
        : mode == 2U                    ? erbsland::cryptology::TlsRecordContentType::Alert
                                        : erbsland::cryptology::TlsRecordContentType::ApplicationData;
    if (payload.empty() && contentType != erbsland::cryptology::TlsRecordContentType::ApplicationData) {
        return 0;
    }
    auto offset = std::size_t{0U};
    do {
        const auto length = std::min<std::size_t>(payload.size() - offset, 1U << 14U);
        const auto content = erbsland::mem::toConstByteSpan(payload.subspan(offset, length));
        const auto record = clientSender.protect(contentType, content);
        feedFragmented(server, record.span(), _data.front());
        offset += length;
    } while (offset < payload.size() && server.state() != erbsland::network::impl::TlsServerProtocolState::Failed);
    return 0;
}

auto TlsServerProtocolFuzz::createClient(bool mutual) -> ClientProtocol {
    using namespace erbsland::text::literals;
    return ClientProtocol{erbsland::network::impl::TlsClientProtocolOptions{
        erbsland::network::Host::fromStringOrThrow("localhost"_el),
        erbsland::cryptology::X509TlsCertificatePolicy{tls_fuzz_fixture::get().certificates},
        erbsland::time::Timestamp::fromTimeT(1'798'761'600),
        {},
        {},
        {erbsland::cryptology::KeyAgreementAlgorithm::X25519},
        mutual ? tls_fuzz_fixture::client().identity : nullptr}};
}

auto TlsServerProtocolFuzz::createServer(bool mutual) -> ServerProtocol {
    auto profile = erbsland::network::impl::TlsServerProtocolConfiguration{tls_fuzz_fixture::get().identity};
    if (mutual) {
        profile.clientAuthentication = erbsland::cryptology::TlsClientAuthentication::Required;
        profile.certificatePolicy =
            erbsland::cryptology::X509TlsCertificatePolicy{tls_fuzz_fixture::client().certificates};
    }
    return ServerProtocol{erbsland::network::impl::TlsServerProtocolOptions{profile}};
}

auto TlsServerProtocolFuzz::establish(ClientProtocol &client, ServerProtocol &server) -> bool {
    for (auto iteration = 0U; iteration < 32U; ++iteration) {
        auto progress = transferClientOutput(client, server);
        if (server.hasCheckpoint()) {
            tls_fuzz_fixture::resume(server);
            progress = true;
        }
        progress = transferServerOutput(server, client) || progress;
        if (client.hasCheckpoint()) {
            tls_fuzz_fixture::resume(client);
            progress = true;
        }
        if (server.checkpoint() == erbsland::network::impl::TlsServerProtocolCheckpoint::HandshakeCompleted) {
            tls_fuzz_fixture::resume(server);
            progress = true;
        }
        if (client.state() == erbsland::network::impl::TlsClientProtocolState::Established &&
            server.state() == erbsland::network::impl::TlsServerProtocolState::Established && !client.hasCheckpoint() &&
            !server.hasCheckpoint()) {
            return true;
        }
        if (!progress || client.state() == erbsland::network::impl::TlsClientProtocolState::Failed ||
            server.state() == erbsland::network::impl::TlsServerProtocolState::Failed) {
            return false;
        }
    }
    return false;
}

auto TlsServerProtocolFuzz::transferClientOutput(ClientProtocol &client, ServerProtocol &server) -> bool {
    auto progress = false;
    while (const auto record = client.takeTransportOutput()) {
        server.feedTransport(record->span());
        progress = true;
    }
    return progress;
}

auto TlsServerProtocolFuzz::transferServerOutput(ServerProtocol &server, ClientProtocol &client) -> bool {
    auto progress = false;
    while (const auto record = server.takeTransportOutput()) {
        client.feedTransport(record->span());
        progress = true;
    }
    return progress;
}

void TlsServerProtocolFuzz::feedFragmented(
    ServerProtocol &protocol, const erbsland::mem::ConstByteSpan data, const uint8_t selector) {
    const auto fragmentLength = std::size_t{1U + selector % 64U};
    auto offset = std::size_t{0U};
    while (offset < data.size()) {
        const auto length = std::min(fragmentLength, data.size() - offset);
        protocol.feedTransport(data.subspan(offset, length));
        offset += length;
    }
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    static const auto application = erbsland::core::Application{};
    static_cast<void>(application);
    return TlsServerProtocolFuzz{std::span<const uint8_t>{data, size}}.run();
}
