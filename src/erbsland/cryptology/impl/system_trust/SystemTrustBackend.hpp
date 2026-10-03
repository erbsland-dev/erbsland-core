#pragma once

#include "../../x509/X509TlsCertificatePolicy.hpp"

#include <atomic>
#include <memory>
#include <optional>

namespace erbsland::cryptology::impl {

/// Synchronous, mockable boundary for native TLS trust evaluation and distribution trust loading.
/// Callers own scheduling and cancellation. Native calls may outlive logical cancellation.
/// Only public certificate data crosses this interface; no secret material is created or retained.
/// @needtest{Native trust integration and backend substitution.}
class SystemTrustBackend {
public:
    // defaults
    virtual ~SystemTrustBackend() = default;

public: // evaluation
    /// Validate system trust at an explicit time.
    /// @param policy Retrieval and bundle selection policy.
    /// @param peerCertificates Target first, followed by untrusted intermediate candidates.
    /// @param referenceIdentity DNS or IP reference authenticated using SAN-only matching.
    /// @param validationTime Time used by the platform chain engine.
    /// @return Structured acceptance or failure, including unavailable trust and native service failures.
    [[nodiscard]] virtual auto validate(
        const X509TlsCertificatePolicy &policy,
        const X509CertificateBundle &peerCertificates,
        const std::optional<network::Host> &referenceIdentity,
        time::Timestamp validationTime) const -> X509CertificateValidation = 0;

public: // factories
    /// Create the native adapter for this platform without performing native work.
    /// @return A stateless adapter; immutable Linux anchor snapshots are cached separately.
    [[nodiscard]] static auto create() -> std::unique_ptr<SystemTrustBackend>;

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
public: // test substitution
    /// Factory used to substitute the native boundary in deterministic integration tests.
    using TestFactory = auto (*)() -> std::unique_ptr<SystemTrustBackend>;
    /// Install a test factory, or restore the native factory with nullptr.
    /// @param factory Process-wide test factory; tests must serialize installation and restore it afterward.
    static void setTestFactory(TestFactory factory) noexcept;
#endif

protected:
    /// Create a structured input-wide rejection.
    [[nodiscard]] static auto reject(X509CertificateValidationFailureCategory category, text::String diagnostic)
        -> X509CertificateValidation;
    /// Apply shared modern-algorithm and SAN policy to an already native-validated path.
    [[nodiscard]] static auto acceptNativePath(
        const X509TlsCertificatePolicy &policy,
        const X509CertificateBundle &peerCertificates,
        const std::optional<network::Host> &referenceIdentity,
        time::Timestamp validationTime,
        util::List<X509Certificate> path,
        X509RevocationStatus revocationStatus) -> X509CertificateValidation;

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
private:
    static std::atomic<TestFactory> _testFactory; ///< Optional deterministic test adapter.
#endif
};

}
