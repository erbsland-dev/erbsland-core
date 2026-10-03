#pragma once

#include "../../../path/Path.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../text/StringMap.hpp"
#include "../../CryptologyError.hpp"
#include "../../x509/X509CertificateBundle.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace erbsland::cryptology::impl {

/// Immutable, fail-closed cache of distribution-maintained certificate bundles.
/// RFC 7468 decoding retains the common 24 MiB text and 16 MiB DER bounds; at most 4096 anchors are indexed.
/// Public certificate bytes require no secret erasure. Replacement snapshots are parsed completely before publication.
/// @tested{SystemTrustTest}
class LinuxTrustBundleCache final {
    /// Immutable metadata used to detect bundle replacement and in-place changes.
    /// @tested{SystemTrustTest}
    struct Metadata final {
        bool regularFile;                ///< Whether the observed source is a regular file.
        system::FileIdentity identity;   ///< Stable filesystem identity.
        unit::ByteCount size;            ///< Observed source size.
        time::Timestamp modified;        ///< Observed data modification time.
        time::Timestamp metadataChanged; ///< Observed filesystem metadata change time.
    };

    /// One completely parsed immutable bundle and its source observation.
    /// @tested{SystemTrustTest}
    struct Entry final {
        Metadata metadata; ///< Immutable file observation; PathInfo itself shares a mutable cache.
        std::shared_ptr<const X509CertificateBundle> bundle; ///< Immutable parsed snapshot.
    };

public: // loading
    /// Load one authoritative bundle, never merging alternatives or retaining a failed replacement.
    /// @param overridePath An explicit bundle, or an empty path for distribution discovery.
    /// @return A shared immutable parsed snapshot.
    /// @throws CryptologyError If discovery fails or the bundle is missing, empty, unreadable, or changes while
    /// loading.
    /// @throws path::PathError If opening or inspecting the bundle fails.
    /// @throws stream::StreamError If reading the bundle fails.
    /// @throws err::ParseError If the bundle contains malformed PEM or certificates.
    [[nodiscard]] static auto load(const path::Path &overridePath) -> std::shared_ptr<const X509CertificateBundle>;

private:
    /// Load or replace a snapshot while holding the cache mutex.
    [[nodiscard]] auto loadLocked(const path::Path &overridePath) -> std::shared_ptr<const X509CertificateBundle>;
    /// Select one distribution-maintained source, without merging alternatives.
    [[nodiscard]] static auto selectDistributionBundle() -> path::Path;
    /// Obtain a fresh filesystem metadata snapshot.
    [[nodiscard]] static auto observe(const path::Path &bundlePath) -> Metadata;
    /// Test whether both observations identify the same unchanged source.
    [[nodiscard]] static auto unchanged(const Metadata &first, const Metadata &second) -> bool;
    /// Parse a bounded distribution bundle, accepting line-leading package comments.
    [[nodiscard]] static auto parse(const path::Path &bundlePath) -> X509CertificateBundle;

private:
    std::mutex _mutex;               ///< Serializes file observation and snapshot publication.
    path::Path _distributionPath;    ///< Selected authoritative distribution path, retained across failures.
    text::StringMap<Entry> _entries; ///< At most sixteen cached explicit bundle paths.
};

}
