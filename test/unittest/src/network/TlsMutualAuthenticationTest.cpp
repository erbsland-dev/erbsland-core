// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/HostName.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TlsIdentity.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(TlsServerProtocol TlsServerProtocolConfiguration X509TlsCertificatePolicy)
class TlsMutualAuthenticationTest final : public el::UnitTest {
    using Authentication = el::cryptology::TlsClientAuthentication;
    using Client = el::network::impl::TlsClientProtocol;
    using Server = el::network::impl::TlsServerProtocol;
    using ClientState = el::network::impl::TlsClientProtocolState;
    using ServerState = el::network::impl::TlsServerProtocolState;
    using Alert = el::network::TlsAlertDescription;
    using Policy = el::cryptology::X509TlsCertificatePolicy;
    using Profile = el::network::impl::TlsServerProtocolConfiguration;
    using Options = el::network::impl::TlsServerProtocolOptions;
    struct Scenario {
        Authentication authentication{Authentication::Required};
        bool identity{true};
        bool corruptSignature{};
        bool wrongTrust{};
        bool fragments{};
        bool retry{};
        bool sniRequired{};
        bool corruptFinished{};
        bool sniWrongTrust{};
        bool coalesce{};
    };
    struct Result {
        bool established{};
        bool authenticated{};
        bool certificateCheckpoint{};
        std::optional<Alert> alert;
    };

private:
    el::String _diagnostic;
    auto additionalErrorMessages() -> std::string override {
        return el::text::StringConverter{_diagnostic}.toStdString();
    }
    static auto bundle(const el::String &name) -> el::cryptology::X509CertificateBundle {
        return el::cryptology::X509CertificateBundle::fromPemOrThrow(
            el::String{el::unittest::fh::readDataText(
                el::text::StringConverter{el::join("data/network/tls-interop/"_el, name)}.toStdString())});
    }
    static auto identity(const el::String &name, const el::String &key) -> el::cryptology::TlsIdentityConstPtr {
        return std::make_shared<const el::cryptology::TlsIdentity>(
            bundle(name),
            el::cryptology::SigningPrivateKey::fromPemOrThrow(
                el::String{el::unittest::fh::readDataText(
                    el::text::StringConverter{el::join("data/network/tls-interop/"_el, key)}.toStdString())}));
    }
    auto run(const Scenario &scenario) -> Result {
        auto scope = ApplicationTestScope<>{};
        scope.app().cryptologyConfiguration().setProtectedDataMode(el::cryptology::ProtectedDataMode::InternalOnly);
        const auto serverIdentity = identity("server-ecdsa.pem"_el, "server-ecdsa-key.pem"_el);
        auto clientIdentity = el::cryptology::TlsIdentityConstPtr{};
        if (scenario.identity) {
            clientIdentity = identity("client.pem"_el, "client-key.pem"_el);
        }
        auto trust = bundle("client-ca.pem"_el);
        if (scenario.wrongTrust) {
            trust = bundle("ca.pem"_el);
        }
        auto profile = Profile{serverIdentity, scenario.authentication, Policy{trust}};
        auto serverOptions = Options{profile};
        if (scenario.retry) {
            serverOptions = Options{
                profile,
                {},
                {el::cryptology::TlsCipherSuite::Aes128GcmSha256},
                {},
                {el::cryptology::KeyAgreementAlgorithm::P256}};
        }
        if (scenario.sniRequired) {
            serverOptions = Options::withNamedConfigurations(
                Profile{serverIdentity},
                {{el::network::HostName::fromStringOrThrow("localhost"_el),
                    Profile{serverIdentity, Authentication::Required, Policy{trust}}}});
        }
        if (scenario.sniWrongTrust) {
            serverOptions = Options::withNamedConfigurations(
                profile,
                {{el::network::HostName::fromStringOrThrow("localhost"_el),
                    Profile{serverIdentity, Authentication::Required, Policy{bundle("ca.pem"_el)}}}});
        }
        auto server = Server{std::move(serverOptions)};
        auto client = Client{el::network::impl::TlsClientProtocolOptions{
            el::network::Host{el::network::HostName::fromStringOrThrow("localhost"_el)},
            Policy{bundle("server-ecdsa.pem"_el)},
            el::time::Timestamp::now(),
            {},
            {},
            {el::cryptology::KeyAgreementAlgorithm::X25519, el::cryptology::KeyAgreementAlgorithm::P256},
            clientIdentity}};
        server.start();
        client.start();
        auto result = Result{};
        auto receiver = el::cryptology::TlsRecordDecryptor{};
        auto sender = el::cryptology::TlsRecordEncryptor{};
        for (auto iteration = 0; iteration < 1000; ++iteration) {
            if (client.hasCheckpoint()) {
                if (const auto request = client.pendingCertificateValidation()) {
                    client.completeCertificateValidation(request->execute());
                } else if (const auto request = client.pendingSignature()) {
                    auto signature = request->execute();
                    if (scenario.corruptSignature) {
                        signature = el::mem::ByteBlock{signature.length(), 0U};
                    }
                    client.completeSignature(std::move(signature));
                } else {
                    client.resume();
                }
            }
            if (server.hasCheckpoint()) {
                if (const auto request = server.pendingCertificateValidation()) {
                    result.certificateCheckpoint = true;
                    REQUIRE_FALSE(server.isPeerAuthenticated());
                    REQUIRE(server.peerCertificatePath().isEmpty());
                    REQUIRE_FALSE(server.hasApplicationData());
                    server.resume();
                    REQUIRE(server.pendingCertificateValidation().has_value());
                    server.completeCertificateValidation(request->execute());
                } else if (const auto request = server.pendingSignature()) {
                    server.completeSignature(request->execute());
                } else {
                    server.resume();
                }
            }
            auto coalesced = el::mem::ByteBlockEditor{};
            while (auto record = client.takeTransportOutput()) {
                if ((scenario.corruptFinished || scenario.coalesce) && record->span().front() == el::mem::Byte{23U}) {
                    if (receiver.isEmpty()) {
                        receiver = el::network::impl::TlsClientProtocolTestAccess{client}.clientHandshakeDecryptor();
                        sender = el::network::impl::TlsServerProtocolTestAccess{server}.clientHandshakeEncryptor();
                    }
                    auto plaintext = receiver.unprotect(record->span());
                    auto content = el::mem::ByteBlockEditor{plaintext.content()};
                    if (scenario.corruptFinished && content.span().front() == el::mem::Byte{20U}) {
                        content.set(content.lastIndex(), content.span().back() ^ el::mem::Byte{1U});
                    }
                    if (scenario.coalesce) {
                        coalesced.append(content);
                        continue;
                    }
                    *record = sender.protect(plaintext.type(), content.span());
                }
                if (scenario.fragments) {
                    for (auto i = std::size_t{}; i < record->span().size(); ++i) {
                        server.feedTransport(record->span().subspan(i, 1U));
                    }
                } else {
                    server.feedTransport(record->span());
                }
            }
            if (!coalesced.isEmpty()) {
                const auto record = sender.protect(el::cryptology::TlsRecordContentType::Handshake, coalesced.span());
                server.feedTransport(record.span());
            }
            while (auto record = server.takeTransportOutput()) {
                client.feedTransport(record->span());
            }
            if (server.state() == ServerState::Failed || client.state() == ClientState::Failed) {
                break;
            }
            if (server.state() == ServerState::Established && !server.hasCheckpoint()) {
                result.established = true;
                break;
            }
        }
        _diagnostic = el::join("Client: "_el, client.failureDiagnostic(), " Server: "_el, server.failureDiagnostic());
        result.authenticated = server.isPeerAuthenticated();
        result.alert = server.failureAlert();
        REQUIRE_EQUAL(server.peerCertificatePath().isEmpty(), !result.authenticated);
        if (result.authenticated) {
            REQUIRE_EQUAL(
                server.peerCertificatePath().first().toDer(),
                clientIdentity->certificateChain().certificates().first().toDer());
            server.close();
            REQUIRE(server.isPeerAuthenticated());
            REQUIRE_FALSE(server.peerCertificatePath().isEmpty());
        }
        return result;
    }

    auto rejectMalformed(const el::mem::ByteBlock &message, bool enabled = true) -> Alert {
        auto scope = ApplicationTestScope<>{};
        scope.app().cryptologyConfiguration().setProtectedDataMode(el::cryptology::ProtectedDataMode::InternalOnly);
        auto profile = Profile{
            identity("server-ecdsa.pem"_el, "server-ecdsa-key.pem"_el),
            Authentication::Required,
            Policy{bundle("client-ca.pem"_el)}};
        if (!enabled) {
            profile.clientAuthentication = Authentication::Disabled;
        }
        auto server = Server{Options{profile}};
        auto client = Client{el::network::impl::TlsClientProtocolOptions{
            el::network::Host::fromStringOrThrow("localhost"_el),
            Policy{bundle("server-ecdsa.pem"_el)},
            el::time::Timestamp::now()}};
        server.start();
        client.start();
        while (const auto record = client.takeTransportOutput()) {
            server.feedTransport(record->span());
        }
        while (server.hasCheckpoint()) {
            if (const auto request = server.pendingSignature()) {
                server.completeSignature(request->execute());
            } else {
                server.resume();
            }
        }
        auto sender = el::network::impl::TlsServerProtocolTestAccess{server}.clientHandshakeEncryptor();
        const auto record = sender.protect(el::cryptology::TlsRecordContentType::Handshake, message.span());
        server.feedTransport(record.span());
        if (const auto request = server.pendingCertificateValidation()) {
            server.completeCertificateValidation(request->execute());
        }
        REQUIRE_FALSE(server.isPeerAuthenticated());
        REQUIRE(server.peerCertificatePath().isEmpty());
        REQUIRE_EQUAL(server.state(), ServerState::Failed);
        REQUIRE(server.failureAlert().has_value());
        return *server.failureAlert();
    }

    static auto certificateMessage(bool context = false, bool extension = false, unsigned count = 1U)
        -> el::mem::ByteBlock {
        using Writer = el::network::impl::TlsWireWriter;
        const auto der = bundle("client.pem"_el).certificates().first().toDer();
        auto entries = Writer{};
        for (auto i = 0U; i < count; ++i) {
            entries.writeVector24(der.span());
            if (extension) {
                entries.writeVector16(el::mem::ByteBlock{0U, 1U, 0U, 0U}.span());
            } else {
                entries.writeVector16({});
            }
        }
        auto body = Writer{};
        if (context) {
            body.writeVector8(el::mem::ByteBlock{1U}.span());
        } else {
            body.writeVector8({});
        }
        body.writeVector24(entries.finish().span());
        const auto bytes = body.finish();
        auto message = Writer{};
        message.writeU8(11U);
        message.writeU24(static_cast<uint32_t>(bytes.length().toSizeT()));
        message.writeBytes(bytes.span());
        return message.finish();
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAuthenticationModes() {
        for (const auto mode : {Authentication::Disabled, Authentication::Optional, Authentication::Required}) {
            for (const auto present : {false, true}) {
                const auto result = run(Scenario{.authentication = mode, .identity = present});
                const auto accepted = present || mode != Authentication::Required;
                REQUIRE_EQUAL(result.established, accepted);
                REQUIRE_EQUAL(result.authenticated, present && mode != Authentication::Disabled);
                if (!accepted) {
                    REQUIRE_EQUAL(result.alert, Alert::CertificateRequired);
                }
            }
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInvalidPresentedCertificatesAndSignaturesFailOptionalAuthentication() {
        const auto wrongTrust = run(Scenario{.authentication = Authentication::Optional, .wrongTrust = true});
        REQUIRE_FALSE(wrongTrust.established);
        REQUIRE_FALSE(wrongTrust.authenticated);
        REQUIRE(wrongTrust.alert.has_value());
        const auto signature = run(Scenario{.authentication = Authentication::Optional, .corruptSignature = true});
        REQUIRE_FALSE(signature.established);
        REQUIRE_FALSE(signature.authenticated);
        REQUIRE_EQUAL(signature.alert, Alert::DecryptError);
        const auto finished = run(Scenario{.corruptFinished = true});
        REQUIRE_FALSE(finished.established);
        REQUIRE_FALSE(finished.authenticated);
        REQUIRE_EQUAL(finished.alert, Alert::DecryptError);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFragmentedCertificateFlightAndHelloRetryRequest() {
        const auto result = run(Scenario{.fragments = true, .retry = true});
        REQUIRE(result.established);
        REQUIRE(result.authenticated);
        REQUIRE(result.certificateCheckpoint);
        const auto coalesced = run(Scenario{.coalesce = true});
        REQUIRE(coalesced.established);
        REQUIRE(coalesced.authenticated);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSniSelectsAuthenticationPolicyAlongWithIdentity() {
        const auto missing = run(Scenario{.identity = false, .sniRequired = true});
        REQUIRE_FALSE(missing.established);
        REQUIRE_EQUAL(missing.alert, Alert::CertificateRequired);
        const auto present = run(Scenario{.sniRequired = true});
        REQUIRE(present.established);
        REQUIRE(present.authenticated);
        const auto isolated = run(Scenario{.sniWrongTrust = true});
        REQUIRE_FALSE(isolated.established);
        REQUIRE_FALSE(isolated.authenticated);
        REQUIRE_EQUAL(isolated.alert, Alert::UnknownCa);
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMalformedAndUnsolicitedClientAuthentication() {
        REQUIRE_EQUAL(rejectMalformed(certificateMessage(true)), Alert::IllegalParameter);
        REQUIRE_EQUAL(rejectMalformed(certificateMessage(false, true)), Alert::UnsupportedExtension);
        REQUIRE_EQUAL(rejectMalformed(certificateMessage(false, false, 17U)), Alert::BadCertificate);
        REQUIRE_EQUAL(rejectMalformed(certificateMessage(false, false, 0U)), Alert::CertificateRequired);
        REQUIRE_EQUAL(rejectMalformed(certificateMessage(), false), Alert::UnexpectedMessage);
        REQUIRE_EQUAL(rejectMalformed(el::mem::ByteBlock{20U, 0U, 0U, 0U}), Alert::UnexpectedMessage);
        auto duplicate = el::mem::ByteBlockEditor{certificateMessage()};
        duplicate.append(certificateMessage());
        REQUIRE_EQUAL(rejectMalformed(duplicate), Alert::UnexpectedMessage);
        auto unadvertised = el::mem::ByteBlockEditor{certificateMessage()};
        unadvertised.append(el::mem::ByteBlock{15U, 0U, 0U, 5U, 4U, 1U, 0U, 1U, 0U});
        REQUIRE_EQUAL(rejectMalformed(unadvertised), Alert::IllegalParameter);
    }
};
