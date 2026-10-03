#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBuffer.hpp>
#include <erbsland/cryptology/impl/system_trust/LinuxSystemTrustBackend.hpp>
#include <erbsland/cryptology/impl/system_trust/LinuxTrustBundleCache.hpp>
#include <erbsland/cryptology/impl/system_trust/SystemTrustBackend.hpp>
#include <erbsland/HostName.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <algorithm>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(
    SystemTrustBackend LinuxSystemTrustBackend LinuxTrustBundleCache X509TlsCertificatePolicy X509RevocationStatus)
class SystemTrustTest final : public el::UnitTest {
    /// Simulate an OS-accepted path to test the remaining common TLS policy.
    class NativePathBackend final : public el::cryptology::impl::SystemTrustBackend {
    public: // implement SystemTrustBackend
        auto validate(
            const X509TlsCertificatePolicy &policy,
            const X509CertificateBundle &peer,
            const std::optional<el::network::Host> &host,
            el::time::Timestamp time) const -> X509CertificateValidation override {
            return acceptNativePath(policy, peer, host, time, peer.certificates(), X509RevocationStatus::Unknown);
        }
    };

public:
    void testNativePathRequiresDigitalSignatureUsage() {
        const auto backend = NativePathBackend{};
        const auto policy = X509TlsCertificatePolicy::systemTrust(false);
        const auto leaf = certificate(2);
        const auto original = X509CertificateBundle{el::util::List<X509Certificate>{leaf}};
        REQUIRE(backend.validate(policy, original, host("server.example.test"_el), leaf.validFrom()).isAccepted());
        auto modified = el::mem::ByteBuffer{leaf.toDer().span()};
        const auto usage =
            el::mem::ByteBuffer{0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02, 0x07, 0x80};
        const auto position =
            std::search(modified.span().begin(), modified.span().end(), usage.span().begin(), usage.span().end());
        REQUIRE(position != modified.span().end());
        const auto offset = static_cast<std::size_t>(position - modified.span().begin());
        modified.set(el::unit::ByteIndex::fromSizeT(offset + 12U), el::mem::Byte{0x05});
        modified.set(el::unit::ByteIndex::fromSizeT(offset + 13U), el::mem::Byte{0x20});
        const auto enciphermentOnly = X509Certificate::fromDerOrThrow(el::mem::ByteBlock::fromSpan(modified.span()));
        REQUIRE(enciphermentOnly.keyUsage()->isSet(X509KeyUsage::KeyEncipherment));
        REQUIRE_FALSE(enciphermentOnly.keyUsage()->isSet(X509KeyUsage::DigitalSignature));
        const auto peer = X509CertificateBundle{el::util::List<X509Certificate>{enciphermentOnly}};
        const auto result = backend.validate(policy, peer, host("server.example.test"_el), leaf.validFrom());
        REQUIRE(result.isRejected());
        REQUIRE(result.failure()->category() == X509CertificateValidationFailureCategory::KeyUsageRejected);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testClientPurposeOnNativeAndLinuxPaths() {
        const auto fixtures = X509CertificateBundle::fromPemOrThrow(
            el::String{el::unittest::fh::readDataText("data/cryptology/x509/client_authentication.pem")},
            X509CertificateProfileMode::Compatible);
        const auto &values = fixtures.certificates().toRawValue();
        const auto peer = X509CertificateBundle{el::util::List<X509Certificate>{values[1], values[0]}};
        const auto policy = X509TlsCertificatePolicy::systemTrust(false);
        const auto time = el::time::Timestamp::fromIsoStringOrThrow("2027-01-01T00:00:00Z"_el);
        const auto backend = NativePathBackend{};
        REQUIRE(backend.validate(policy, peer, std::nullopt, time).isAccepted());
        REQUIRE(backend
                .validate(
                    policy,
                    X509CertificateBundle{el::util::List<X509Certificate>{values[2], values[0]}},
                    std::nullopt,
                    time)
                .isRejected());
        REQUIRE(backend
                .validate(
                    policy,
                    X509CertificateBundle{el::util::List<X509Certificate>{values[4], values[0]}},
                    std::nullopt,
                    time)
                .isRejected());
        const auto application = ApplicationTestScope<>{};
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto file = directory->path() / "client-roots.pem"_el;
        file.content().writeTextOrThrow(values[0].toPem());
        const auto linuxPolicy = X509TlsCertificatePolicy::systemTrust(false, file);
        REQUIRE(
            el::cryptology::impl::LinuxSystemTrustBackend{}
                .validate(linuxPolicy, peer, std::nullopt, time)
                .isAccepted());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSystemTrustRejectsUntrustedClientOffline() {
        const auto application = ApplicationTestScope<>{};
        const auto peer = X509CertificateBundle::fromPemOrThrow(
            el::String{el::unittest::fh::readDataText("data/network/tls-interop/client.pem")});
        const auto result = X509TlsCertificatePolicy::systemTrust(false).validateClient(peer);
        REQUIRE(result.isRejected());
        REQUIRE(result.failure()->category() != X509CertificateValidationFailureCategory::NativeFailure);
    }

    void testSystemPolicyIsLazyAndExplicit() {
        const auto policy = X509TlsCertificatePolicy::systemTrust(false, el::path::Path{"missing-bundle.pem"_el});
        REQUIRE(policy.isSystemTrust());
        REQUIRE_FALSE(policy.allowsNetworkRetrieval());
        REQUIRE_EQUAL(policy.systemBundlePath().toString(), "missing-bundle.pem"_el);
        REQUIRE(policy.trustAnchors().isEmpty());
        REQUIRE_FALSE(X509TlsCertificatePolicy{X509CertificateBundle{}}.isSystemTrust());
    }

    void testBundleCacheReplacementFailsClosed() {
        const auto application = ApplicationTestScope<>{};
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto file = directory->path() / "trust.pem"_el;
        file.content().writeTextOrThrow(certificate(0).toPem());
        const auto first = el::cryptology::impl::LinuxTrustBundleCache::load(file);
        REQUIRE(first == el::cryptology::impl::LinuxTrustBundleCache::load(file));
        file.operations().removeOrThrow();
        file.content().writeTextOrThrow("malformed replacement"_el);
        REQUIRE_THROWS_AS(el::err::RuntimeError, el::cryptology::impl::LinuxTrustBundleCache::load(file));
        REQUIRE_THROWS_AS(el::err::RuntimeError, el::cryptology::impl::LinuxTrustBundleCache::load(file));
        file.operations().removeOrThrow();
        file.content().writeTextOrThrow(certificate(5).toPem());
        const auto replacement = el::cryptology::impl::LinuxTrustBundleCache::load(file);
        REQUIRE(first != replacement);
        REQUIRE_EQUAL(first->certificates().first().toDer(), certificate(0).toDer());
        REQUIRE_EQUAL(replacement->certificates().first().toDer(), certificate(5).toDer());
        file.operations().removeOrThrow();
        file.content().writeTextOrThrow(""_el);
        REQUIRE_THROWS_AS(el::err::RuntimeError, el::cryptology::impl::LinuxTrustBundleCache::load(file));
        file.operations().removeOrThrow();
        REQUIRE_THROWS_AS(el::err::RuntimeError, el::cryptology::impl::LinuxTrustBundleCache::load(file));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPortableTrustAndSanMismatch() {
        const auto application = ApplicationTestScope<>{};
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto file = directory->path() / "trust.pem"_el;
        auto pem = el::text::StringEditor{"# Distribution certificate label\n"_el};
        pem.append(certificate(0).toPem());
        file.content().writeTextOrThrow(el::text::String{pem});
        const auto policy = X509TlsCertificatePolicy::systemTrust(false, file);
        const auto peer = X509CertificateBundle{el::util::List<X509Certificate>{certificate(2), certificate(1)}};
        const auto backend = el::cryptology::impl::LinuxSystemTrustBackend{};
        const auto accepted =
            backend.validate(policy, peer, host("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(accepted.isAccepted());
        REQUIRE(accepted.revocationStatus() == X509RevocationStatus::NotChecked);
        const auto rejected = backend.validate(policy, peer, host("wrong.example.test"_el), el::time::Timestamp::now());
        REQUIRE(rejected.isRejected());
        REQUIRE(rejected.failure()->category() == X509CertificateValidationFailureCategory::ServerIdentityMismatch);
        const auto unavailable = backend.validate(
            X509TlsCertificatePolicy::systemTrust(false, directory->path() / "missing"_el),
            peer,
            host("server.example.test"_el),
            el::time::Timestamp::now());
        REQUIRE(unavailable.failure()->category() == X509CertificateValidationFailureCategory::TrustUnavailable);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLargeTrustBundleDoesNotConsumePeerBudget() {
        const auto application = ApplicationTestScope<>{};
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto file = directory->path() / "trust.pem"_el;
        auto pem = el::text::StringEditor{};
        for (auto index = std::size_t{}; index < 300U; ++index) {
            pem.append(certificate(0).toPem());
        }
        file.content().writeTextOrThrow(el::text::String{pem});
        const auto anchors = el::cryptology::impl::LinuxTrustBundleCache::load(file);
        REQUIRE_EQUAL(anchors->certificates().count(), el::unit::ItemCount{300U});
        const auto peer = X509CertificateBundle{el::util::List<X509Certificate>{certificate(2), certificate(1)}};
        const auto result = X509TlsCertificatePolicy{*anchors}.validateServer(peer, host("server.example.test"_el));
        REQUIRE(result.isAccepted());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNativePathStillRequiresSanAndModernAlgorithms() {
        const auto backend = NativePathBackend{};
        const auto policy = X509TlsCertificatePolicy::systemTrust(false);
        const auto peer =
            X509CertificateBundle{el::util::List<X509Certificate>{certificate(2), certificate(1), certificate(0)}};
        const auto accepted =
            backend.validate(policy, peer, host("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(accepted.isAccepted());
        REQUIRE(accepted.revocationStatus() == X509RevocationStatus::Unknown);
        REQUIRE(backend.validate(policy, peer, host("wrong.example.test"_el), el::time::Timestamp::now()).isRejected());
        const auto commonNameOnly = X509CertificateBundle{el::util::List<X509Certificate>{certificate(4)}};
        REQUIRE(backend.validate(policy, commonNameOnly, host("server.example.test"_el), el::time::Timestamp::now())
                .isRejected());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSystemTrustRejectsUntrustedFixtureOffline() {
        const auto peer =
            X509CertificateBundle{el::util::List<X509Certificate>{certificate(2), certificate(1), certificate(0)}};
        const auto result =
            X509TlsCertificatePolicy::systemTrust(false).validateServer(peer, host("server.example.test"_el));
        REQUIRE(result.isRejected());
        REQUIRE(result.failure()->category() != X509CertificateValidationFailureCategory::NativeFailure);
        REQUIRE(result.failure()->category() != X509CertificateValidationFailureCategory::TrustUnavailable);
    }

private:
    static auto certificate(std::size_t index) -> X509Certificate {
        static const auto bundle = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/cryptology/x509/server_authentication.pem")});
        return bundle.certificates().get(el::unit::ItemIndex::fromSizeT(index));
    }
    static auto host(const el::text::String &name) -> el::network::Host {
        return el::network::HostName::fromStringOrThrow(name);
    }
};
