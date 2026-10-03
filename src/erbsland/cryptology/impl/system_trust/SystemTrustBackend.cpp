#include "SystemTrustBackend.hpp"

#include "../X509TlsCertificateValidator.hpp"

#include "../../../core/Definitions.hpp"

#if defined(ERBSLAND_OS_MACOS)
#include "MacosSystemTrustBackend.hpp"
#elif defined(ERBSLAND_OS_WINDOWS)
#include "WindowsSystemTrustBackend.hpp"
#else
#include "LinuxSystemTrustBackend.hpp"
#endif

#include <optional>

namespace erbsland::cryptology::impl {

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
std::atomic<SystemTrustBackend::TestFactory> SystemTrustBackend::_testFactory{nullptr};

void SystemTrustBackend::setTestFactory(const TestFactory factory) noexcept {
    _testFactory.store(factory, std::memory_order_release);
}

#endif

auto SystemTrustBackend::create() -> std::unique_ptr<SystemTrustBackend> {
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    if (const auto factory = _testFactory.load(std::memory_order_acquire); factory != nullptr) {
        return factory();
    }
#endif
#if defined(ERBSLAND_OS_MACOS)
    return std::make_unique<MacosSystemTrustBackend>();
#elif defined(ERBSLAND_OS_WINDOWS)
    return std::make_unique<WindowsSystemTrustBackend>();
#else
    return std::make_unique<LinuxSystemTrustBackend>();
#endif
}

auto SystemTrustBackend::reject(X509CertificateValidationFailureCategory category, text::String diagnostic)
    -> X509CertificateValidation {
    return X509CertificateValidation{X509CertificateValidationFailure{category, {}, {}, {}, std::move(diagnostic)}};
}

auto SystemTrustBackend::acceptNativePath(
    const X509TlsCertificatePolicy &policy,
    const X509CertificateBundle &peerCertificates,
    const std::optional<network::Host> &referenceIdentity,
    time::Timestamp validationTime,
    util::List<X509Certificate> path,
    X509RevocationStatus revocationStatus) -> X509CertificateValidation {
    return X509TlsCertificateValidator{policy, peerCertificates, referenceIdentity, validationTime}.validateNativePath(
        std::move(path), revocationStatus);
}

}
