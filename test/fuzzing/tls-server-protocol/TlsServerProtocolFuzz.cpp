// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordContentType.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/network/Host.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolTestAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/DateTime.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

class TlsServerProtocolFuzzInput final {
private:
    using ClientProtocol = erbsland::network::impl::TlsClientProtocol;
    using ServerProtocol = erbsland::network::impl::TlsServerProtocol;
    using ServerTestAccess = erbsland::network::impl::TlsServerProtocolTestAccess;

    struct TestIdentity {
        erbsland::cryptology::X509CertificateBundle certificates;
        erbsland::cryptology::TlsServerIdentityConstPtr identity;
    };

public:
    explicit TlsServerProtocolFuzzInput(const std::span<const uint8_t> data) : _data{data} {}

    [[nodiscard]] auto run() const -> int {
        static constexpr auto cMaximumInputLength = std::size_t{64U * 1024U};
        if (_data.empty() || _data.size() > cMaximumInputLength) {
            return 0;
        }

        const auto mode = _data.front() % 5U;
        const auto payload = _data.subspan(1U);
        auto server = createServer();
        server.start();
        if (mode == 0U) {
            feedFragmented(server, erbsland::mem::toConstByteSpan(payload), _data.front());
            server.abort();
            return 0;
        }

        auto client = createClient();
        client.start();
        if (!establish(client, server)) {
            return 0;
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

private:
    [[nodiscard]] static auto testIdentity() -> const TestIdentity & {
        using erbsland::text::String;
        using namespace erbsland::text::literals;
        static const auto result = []() -> TestIdentity {
            static constexpr auto cCertificate = R"PEM(-----BEGIN CERTIFICATE-----
MIIBOjCB7aADAgECAhQbMGnPk+uRDdUlVQ/WTX/UiQbF2jAFBgMrZXAwFDESMBAG
A1UEAwwJbG9jYWxob3N0MB4XDTI2MDgyMDA3MzExMVoXDTM2MDgxODA3MzExMVow
FDESMBAGA1UEAwwJbG9jYWxob3N0MCowBQYDK2VwAyEAFs6YtIaX6wy6BZ94JQi4
l6Rw+U23D90oV/1MpY7QGqejUTBPMAwGA1UdEwEB/wQCMAAwDgYDVR0PAQH/BAQD
AgeAMBMGA1UdJQQMMAoGCCsGAQUFBwMBMBoGA1UdEQQTMBGCCWxvY2FsaG9zdIcE
fwAAATAFBgMrZXADQQDCvxnKSOrrBNDvvXGjB18tu7U/zJHSbnJnD3b2OUu2ZB0M
/QuaBFG+UQA6kQMGJDlSDZOF4AcTsTuPKgnD4joC
-----END CERTIFICATE-----
)PEM"_el;
            static constexpr auto cPrivateKey = R"PEM(-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIGdkQSn1H8KGGlCkycDUBKxtm8xI+ri1WfZ5EUeHXPJD
-----END PRIVATE KEY-----
)PEM"_el;
            auto certificates = erbsland::cryptology::X509CertificateBundle::fromPemOrThrow(String{cCertificate});
            auto identity = std::make_shared<const erbsland::cryptology::TlsServerIdentity>(
                certificates, erbsland::cryptology::SigningPrivateKey::fromPemOrThrow(String{cPrivateKey}));
            return TestIdentity{std::move(certificates), std::move(identity)};
        }();
        return result;
    }

    [[nodiscard]] static auto createClient() -> ClientProtocol {
        using namespace erbsland::text::literals;
        return ClientProtocol{erbsland::network::impl::TlsClientProtocolOptions{
            erbsland::network::Host::fromStringOrThrow("localhost"_el),
            erbsland::cryptology::X509ServerCertificatePolicy{testIdentity().certificates},
            erbsland::time::DateTime::now()}};
    }

    [[nodiscard]] static auto createServer() -> ServerProtocol {
        return ServerProtocol{erbsland::network::impl::TlsServerProtocolOptions{testIdentity().identity}};
    }

    [[nodiscard]] static auto establish(ClientProtocol &client, ServerProtocol &server) -> bool {
        for (auto iteration = 0U; iteration < 32U; ++iteration) {
            auto progress = transferClientOutput(client, server);
            if (server.checkpoint() == erbsland::network::impl::TlsServerProtocolCheckpoint::ClientHello) {
                server.resume();
                progress = true;
            }
            progress = transferServerOutput(server, client) || progress;
            while (client.hasCheckpoint()) {
                client.resume();
                progress = true;
            }
            if (server.checkpoint() == erbsland::network::impl::TlsServerProtocolCheckpoint::HandshakeCompleted) {
                server.resume();
                progress = true;
            }
            if (client.state() == erbsland::network::impl::TlsClientProtocolState::Established &&
                server.state() == erbsland::network::impl::TlsServerProtocolState::Established &&
                !client.hasCheckpoint() && !server.hasCheckpoint()) {
                return true;
            }
            if (!progress || client.state() == erbsland::network::impl::TlsClientProtocolState::Failed ||
                server.state() == erbsland::network::impl::TlsServerProtocolState::Failed) {
                return false;
            }
        }
        return false;
    }

    [[nodiscard]] static auto transferClientOutput(ClientProtocol &client, ServerProtocol &server) -> bool {
        auto progress = false;
        while (const auto record = client.takeTransportOutput()) {
            server.feedTransport(record->span());
            progress = true;
        }
        return progress;
    }

    [[nodiscard]] static auto transferServerOutput(ServerProtocol &server, ClientProtocol &client) -> bool {
        auto progress = false;
        while (const auto record = server.takeTransportOutput()) {
            client.feedTransport(record->span());
            progress = true;
        }
        return progress;
    }

    static void feedFragmented(
        ServerProtocol &protocol, const erbsland::mem::ConstByteSpan data, const uint8_t selector) {
        const auto fragmentLength = std::size_t{1U + selector % 64U};
        auto offset = std::size_t{0U};
        while (offset < data.size()) {
            const auto length = std::min(fragmentLength, data.size() - offset);
            protocol.feedTransport(data.subspan(offset, length));
            offset += length;
        }
    }

private:
    std::span<const uint8_t> _data; ///< Mode/fragment selector followed by bounded arbitrary bytes.
};

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    static const auto application = erbsland::core::Application{};
    static_cast<void>(application);
    return TlsServerProtocolFuzzInput{std::span<const uint8_t>{data, size}}.run();
}
