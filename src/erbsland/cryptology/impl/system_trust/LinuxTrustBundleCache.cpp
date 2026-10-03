#include "LinuxTrustBundleCache.hpp"

#include "../PemDerFileTools.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../CryptologyError.hpp"

#include <array>

namespace erbsland::cryptology::impl {

using namespace path;
using namespace text::literals;

auto LinuxTrustBundleCache::load(const Path &overridePath) -> std::shared_ptr<const X509CertificateBundle> {
    static auto cache = LinuxTrustBundleCache{};
    const auto lock = std::scoped_lock{cache._mutex};
    return cache.loadLocked(overridePath);
}

auto LinuxTrustBundleCache::loadLocked(const Path &overridePath) -> std::shared_ptr<const X509CertificateBundle> {
    auto selected = overridePath;
    if (selected.isEmpty()) {
        if (_distributionPath.isEmpty()) {
            _distributionPath = selectDistributionBundle();
        }
        selected = _distributionPath;
    }
    const auto key = selected.toString();
    const auto before = observe(selected);
    if (!before.regularFile || !before.identity.isValid() || before.size.isZero()) {
        _entries.remove(key);
        throw CryptologyError{
            "System trust bundle is missing, empty, or unreadable; install the distribution ca-certificates package or configure an explicit bundle path."_el};
    }
    if (const auto existing = _entries.get(key); existing && unchanged(before, existing->metadata)) {
        return existing->bundle;
    }
    // Fail closed: a changed or malformed source must never resurrect the preceding trust generation.
    _entries.remove(key);
    auto replacement = std::make_shared<const X509CertificateBundle>(parse(selected));
    const auto after = observe(selected);
    if (!unchanged(before, after)) {
        throw CryptologyError{
            "System trust bundle changed during loading; retry with a stable distribution bundle."_el};
    }
    if (_entries.count() >= unit::ItemCount{16U}) {
        _entries.remove(_entries.first().first);
    }
    _entries.set(key, Entry{after, replacement});
    return replacement;
}

auto LinuxTrustBundleCache::observe(const Path &bundlePath) -> Metadata {
    const auto parts =
        PathInfoParts{PathInfoPart::Type} | PathInfoPart::Size | PathInfoPart::Times | PathInfoPart::FileIdentity;
    auto information = Path{bundlePath.toString()}.info(parts);
    information.reload(parts);
    return Metadata{
        information.isRegularFile(),
        information.fileIdentity(),
        information.fileSize(),
        information.lastModified(),
        information.lastMetadataChange()};
}

auto LinuxTrustBundleCache::unchanged(const Metadata &first, const Metadata &second) -> bool {
    return second.regularFile && first.identity == second.identity && first.size == second.size &&
        first.modified == second.modified && first.metadataChanged == second.metadataChanged;
}

auto LinuxTrustBundleCache::selectDistributionBundle() -> Path {
    // Debian/Ubuntu and Arch, Fedora/RHEL, SUSE, then the extracted p11-kit bundle used by recent distributions.
    const auto locations = std::array{
        "/etc/ssl/certs/ca-certificates.crt"_el,
        "/etc/pki/tls/certs/ca-bundle.crt"_el,
        "/etc/ssl/ca-bundle.pem"_el,
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"_el};
    for (const auto &location : locations) {
        const auto candidate = Path{location};
        if (candidate.info().exists()) {
            return candidate;
        }
    }
    throw CryptologyError{
        "No distribution TLS CA bundle was found; install ca-certificates or configure an explicit bundle path."_el};
}

auto LinuxTrustBundleCache::parse(const Path &bundlePath) -> X509CertificateBundle {
    const auto bytes = PemDerFileTools{bundlePath, PemDerFileTools::Artifact::Certificate}.read();
    const auto source = PemDerFileTools::toPemText(bytes);
    auto reader = text::StringCharReader{source};
    auto pem = text::StringEditor{};
    auto lineStart = true;
    auto comment = false;
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (lineStart && character == U'#') {
            comment = true;
        }
        if (!comment) {
            pem.append(character);
        }
        lineStart = character == U'\n';
        if (lineStart) {
            if (comment) {
                pem.append(U'\n');
            }
            comment = false;
        }
    }
    return X509CertificateBundle::fromPemOrThrow(pem, X509CertificateProfileMode::Compatible);
}

}
